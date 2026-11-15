// Load test for Micro-Vector Editor: signs users in, shares documents among them, then has each editor
// stream edits (moves of its own shape) and/or pointer moves over the real WebSocket protocol.
//
//   node loadtest.mjs --base http://127.0.0.1:8100 --docs 1 --editors 10 --viewers 0 \
//        --opRate 20 --cursorRate 0 --seconds 20
//
// Reports: ack latency (send -> ack), fan-out latency (send -> another member receives the op),
// acked edits per second, rejects by reason, and messages/bytes received per second by type.

const args = Object.fromEntries(
  process.argv.slice(2).reduce((pairs, a, i, all) => (a.startsWith('--') ? [...pairs, [a.slice(2), all[i + 1]]] : pairs), []),
)
const base = args.base ?? 'http://127.0.0.1:8100'
const docs = Number(args.docs ?? 1)
const editors = Number(args.editors ?? 5) // per document
const viewers = Number(args.viewers ?? 0) // per document; connect and join but don't edit
const opRate = Number(args.opRate ?? 20) // edits/s per editor
const cursorRate = Number(args.cursorRate ?? 0) // pointer moves/s per member (editors and viewers)
const seconds = Number(args.seconds ?? 20)
const label = args.label ?? ''
const run = Math.random().toString(36).slice(2, 8)

const wsBase = base.replace(/^http/, 'ws')
const post = async (path, body, token) => {
  const res = await fetch(base + path, {
    method: 'POST',
    headers: { 'content-type': 'application/json', ...(token ? { authorization: `Bearer ${token}` } : {}) },
    body: JSON.stringify(body),
  })
  const text = await res.text()
  if (!res.ok) throw new Error(`${path} ${res.status} ${text}`)
  return JSON.parse(text)
}

async function makeUser(i) {
  const email = `lt-${run}-${i}@load.test`
  const password = 'load-test-password'
  await post('/api/auth/signup', { email, password })
  const { accessToken } = await post('/api/auth/login', { email, password })
  return { email, token: accessToken }
}

// Bounded concurrency, so signups don't just queue on the 2 hashing threads per server.
async function mapLimit(n, limit, fn) {
  const out = new Array(n)
  let next = 0
  await Promise.all(
    Array.from({ length: Math.min(limit, n) }, async () => {
      while (next < n) {
        const i = next++
        out[i] = await fn(i)
      }
    }),
  )
  return out
}

const pct = (sorted, p) => (sorted.length ? sorted[Math.min(sorted.length - 1, Math.floor((p / 100) * sorted.length))] : NaN)
const fmt = (x) => (Number.isFinite(x) ? x.toFixed(1) : '-')

// ---- setup ----
const perDoc = editors + viewers
const t0 = Date.now()
const users = await mapLimit(docs * perDoc, 16, makeUser)
const authMs = Date.now() - t0

const groups = []
for (let d = 0; d < docs; d++) {
  const members = users.slice(d * perDoc, (d + 1) * perDoc)
  const owner = members[0]
  const doc = await post('/api/docs', { title: `load ${run} ${d}` }, owner.token)
  await mapLimit(members.length - 1, 8, (k) =>
    post(`/api/docs/${doc.id}/members`, { email: members[k + 1].email, role: k + 1 < editors ? 'editor' : 'viewer' }, owner.token),
  )
  groups.push({ docId: doc.id, members })
}

// ---- measurement state ----
const sentAt = new Map() // clientOpId -> ms
const ackLat = []
const fanLat = []
const rejects = {}
const recv = {} // type -> {count, bytes}
let measuring = false
let sentOps = 0
let acked = 0
let closedEarly = 0
let joinMs = []

function connect(group, user, index, isEditor) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(`${wsBase}/ws/docs/${group.docId}?token=${encodeURIComponent(user.token)}`)
    const client = { ws, shapeId: `lt_${run}_${group.docId.slice(0, 8)}_${index}`, ready: false, timers: [] }
    let openedAt = Date.now()
    ws.onopen = () => (openedAt = Date.now())
    ws.onerror = (e) => reject(new Error('ws error ' + (e.message ?? '')))
    ws.onclose = (e) => {
      client.timers.forEach(clearInterval)
      if (!client.done) {
        closedEarly++
        if (!client.ready) reject(new Error(`closed ${e.code} ${e.reason}`))
      }
    }
    ws.onmessage = (event) => {
      const text = String(event.data)
      const m = JSON.parse(text)
      if (measuring) {
        const r = (recv[m.type] ??= { count: 0, bytes: 0 })
        r.count++
        r.bytes += text.length
      }
      switch (m.type) {
        case 'ready':
          ws.send(JSON.stringify({ type: 'join', lastSeq: 0 }))
          break
        case 'joined':
          joinMs.push(Date.now() - openedAt)
          client.ready = true
          resolve(client)
          break
        case 'ack': {
          const s = sentAt.get(m.clientOpId)
          if (s !== undefined) {
            if (measuring) {
              ackLat.push(performance.now() - s)
              acked++
            }
            client.lastSeq = m.seq
          }
          if (m.clientOpId === client.addId) client.added = true
          break
        }
        case 'op': {
          const s = sentAt.get(m.clientOpId)
          if (measuring && s !== undefined) fanLat.push(performance.now() - s)
          break
        }
        case 'reject':
          if (measuring) rejects[m.reason] = (rejects[m.reason] ?? 0) + 1
          if (m.clientOpId === client.addId) client.addFailed = m.reason
          break
      }
    }
    client.isEditor = isEditor
  })
}

let opCounter = 0
function sendOp(client, op) {
  const clientOpId = `${run}-${opCounter++}`
  sentAt.set(clientOpId, performance.now())
  client.ws.send(JSON.stringify({ type: 'op', clientOpId, baseSeq: client.lastSeq ?? 0, op }))
  return clientOpId
}

const clients = []
const t1 = Date.now()
for (const group of groups) {
  const joined = await Promise.all(group.members.map((u, i) => connect(group, u, i, i < editors)))
  clients.push(...joined)
}
const connectMs = Date.now() - t1

// Each editor adds its own rectangle, then only ever moves it, so its edits never conflict with others'.
for (const c of clients.filter((c) => c.isEditor)) {
  c.addId = sendOp(c, {
    kind: 'addShape',
    id: c.shapeId,
    shape: { kind: 'rect', x: 10, y: 10, width: 20, height: 20, cornerRadius: 0, style: { stroke: 'black', fill: 'none', strokeWidth: 1 } },
  })
}
const addDeadline = Date.now() + 15000
while (clients.some((c) => c.isEditor && !c.added && !c.addFailed) && Date.now() < addDeadline) await new Promise((r) => setTimeout(r, 50))
const notAdded = clients.filter((c) => c.isEditor && !c.added).length

// ---- run ----
const cpuStart = process.cpuUsage()
measuring = true
const runStart = performance.now()
for (const c of clients) {
  c.timers.push(setInterval(() => c.ws.readyState === 1 && c.ws.send('{"type":"ping"}'), 15000))
  if (c.isEditor && opRate > 0)
    c.timers.push(
      setInterval(() => {
        if (c.ws.readyState !== 1) return
        sentOps++
        sendOp(c, { kind: 'move', id: c.shapeId, dx: 0.5, dy: -0.5 })
      }, 1000 / opRate),
    )
  if (cursorRate > 0)
    c.timers.push(
      setInterval(() => c.ws.readyState === 1 && c.ws.send(JSON.stringify({ type: 'cursor', x: Math.random() * 800, y: Math.random() * 600 })), 1000 / cursorRate),
    )
}
await new Promise((r) => setTimeout(r, seconds * 1000))
for (const c of clients) c.timers.forEach(clearInterval)
const sendWindow = (performance.now() - runStart) / 1000
await new Promise((r) => setTimeout(r, 3000)) // let in-flight acks land
measuring = false
const cpu = process.cpuUsage(cpuStart)
for (const c of clients) {
  c.done = true
  c.ws.close()
}

// ---- report ----
ackLat.sort((a, b) => a - b)
fanLat.sort((a, b) => a - b)
const elapsed = sendWindow + 3
const report = {
  label,
  config: { docs, editorsPerDoc: editors, viewersPerDoc: viewers, opRate, cursorRate, seconds, connections: clients.length },
  setup: { users: users.length, authMs, connectMs, joinP50Ms: pct(joinMs.sort((a, b) => a - b), 50), shapesNotAdded: notAdded },
  offeredOpsPerSec: +(sentOps / sendWindow).toFixed(0),
  ackedOpsPerSec: +(acked / sendWindow).toFixed(0),
  sent: sentOps,
  acked,
  rejects,
  ackLatencyMs: { p50: +fmt(pct(ackLat, 50)), p95: +fmt(pct(ackLat, 95)), p99: +fmt(pct(ackLat, 99)), max: +fmt(ackLat.at(-1)) },
  fanoutLatencyMs: { p50: +fmt(pct(fanLat, 50)), p99: +fmt(pct(fanLat, 99)) },
  receivedPerSec: Object.fromEntries(
    Object.entries(recv).map(([t, r]) => [t, { msgs: Math.round(r.count / elapsed), kb: Math.round(r.bytes / elapsed / 1024) }]),
  ),
  closedEarly,
  loadGenCpuPct: +(((cpu.user + cpu.system) / 1e6 / elapsed) * 100).toFixed(0),
}
console.log(JSON.stringify(report))
process.exit(0)
