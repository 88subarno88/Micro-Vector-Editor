import { expect, type APIRequestContext, type Browser, type Page } from '@playwright/test'

export interface User {
  email: string
  token: string
}

const password = 'correct horse battery'

// A new user, signed up and logged in through the API. Emails are unique, so tests never share a user.
export async function newUser(request: APIRequestContext, name: string): Promise<User> {
  const email = `${name}-${Date.now()}-${Math.floor(Math.random() * 1e6)}@example.com`
  expect((await request.post('/api/auth/signup', { data: { email, password } })).status()).toBe(201)
  const login = await request.post('/api/auth/login', { data: { email, password } })
  expect(login.status()).toBe(200)
  return { email, token: (await login.json()).accessToken }
}

// A document owned by `owner`, which each of `editors` may edit too. Its ID.
export async function sharedDocument(request: APIRequestContext, owner: User, editors: User[], title: string) {
  const headers = { Authorization: `Bearer ${owner.token}` }
  const created = await request.post('/api/docs', { data: { title }, headers })
  expect(created.status()).toBe(201)
  const { id } = await created.json()
  for (const editor of editors) {
    const shared = await request.post(`/api/docs/${id}/members`, {
      data: { email: editor.email, role: 'editor' },
      headers,
    })
    expect(shared.status()).toBe(201)
  }
  return id as string
}

// Logs `user` in, in a browser of their own, and opens the document called `title` once it's live.
export async function openAs(browser: Browser, user: User, title: string): Promise<Page> {
  const page = await (await browser.newContext({ viewport: { width: 1200, height: 900 } })).newPage()
  await logIn(page, user)
  await page.getByRole('link', { name: title }).click()
  await expect(page.getByRole('status')).toHaveText('Connected')
  return page
}

export async function logIn(page: Page, user: User) {
  await page.goto('/')
  await page.getByLabel('Email').fill(user.email)
  await page.getByLabel('Password').fill(password)
  await page.getByRole('button', { name: 'Log in' }).click()
  await expect(page.getByRole('heading', { name: 'Your documents' })).toBeVisible()
}

// A point on the drawing, as fractions of its width and height, in page coordinates.
export async function onDrawing(page: Page, x: number, y: number): Promise<[number, number]> {
  const box = (await page.getByRole('img', { name: 'The drawing' }).boundingBox())!
  return [box.x + box.width * x, box.y + box.height * y]
}

export async function dragOnDrawing(page: Page, from: [number, number], to: [number, number], steps = 5) {
  await page.mouse.move(...(await onDrawing(page, ...from)))
  await page.mouse.down()
  await page.mouse.move(...(await onDrawing(page, ...to)), { steps })
  await page.mouse.up()
}

// What's drawn, as a number: two canvases showing the same shapes give the same one. 0 for an empty drawing.
// Only dark pixels count, so the blue outline round a selected shape, which only its selector sees, doesn't.
export const picture = (page: Page) =>
  page.evaluate(() => {
    const canvas = document.querySelector('canvas')!
    const { data } = canvas.getContext('2d')!.getImageData(0, 0, canvas.width, canvas.height)
    let hash = 0
    for (let i = 0; i < data.length; i += 4) {
      if (data[i] < 180 && data[i + 1] < 180 && data[i + 2] < 180) hash = (hash * 31 + i) | 0
    }
    return hash
  })

// The leftmost dark pixel in the row across the drawing at `y` (a fraction of its height), or null.
export const leftEdgeAt = (page: Page, y: number) =>
  page.evaluate((y) => {
    const canvas = document.querySelector('canvas')!
    const row = canvas.getContext('2d')!.getImageData(0, Math.round(canvas.height * y), canvas.width, 1).data
    for (let x = 0; x < canvas.width; x++)
      if (row[x * 4] < 180 && row[x * 4 + 1] < 180 && row[x * 4 + 2] < 180) return x
    return null
  }, y)
