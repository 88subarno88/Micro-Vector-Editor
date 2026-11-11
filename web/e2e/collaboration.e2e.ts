// Two people editing one document, each in a browser of their own.
import { expect, test, type Page } from '@playwright/test'
import { dragOnDrawing, leftEdgeAt, newUser, onDrawing, openAs, picture, sharedDocument } from './helpers'

let alice: Page
let bob: Page

test.beforeEach(async ({ browser, request }) => {
  const [a, b] = [await newUser(request, 'alice'), await newUser(request, 'bob')]
  await sharedDocument(request, a, [b], 'Shared plan')
  alice = await openAs(browser, a, 'Shared plan')
  bob = await openAs(browser, b, 'Shared plan')
})

// Waits until both show the same drawing, and returns it.
async function sameDrawing() {
  await expect.poll(async () => (await picture(alice)) === (await picture(bob))).toBe(true)
  return picture(alice)
}

test('a shape drawn by one appears for the other', async () => {
  expect(await sameDrawing()).toBe(0)
  await alice.getByRole('button', { name: 'Rectangle', exact: true }).click()
  await dragOnDrawing(alice, [0.2, 0.25], [0.3, 0.4])
  expect(await sameDrawing()).not.toBe(0)
})

test('a shape being dragged is seen moving before it is let go', async () => {
  await alice.getByRole('button', { name: 'Rectangle', exact: true }).click()
  await dragOnDrawing(alice, [0.2, 0.25], [0.3, 0.4])
  await alice.getByRole('button', { name: 'Select', exact: true }).click()
  await sameDrawing()
  const start = (await leftEdgeAt(bob, 0.32))!

  await alice.mouse.move(...(await onDrawing(alice, 0.25, 0.32)))
  await alice.mouse.down()
  const seen: (number | null)[] = []
  for (let i = 1; i <= 20; i++) {
    await alice.mouse.move(...(await onDrawing(alice, 0.25 + i * 0.01, 0.32)))
    await alice.waitForTimeout(30)
    if (i % 5 === 0) seen.push(await leftEdgeAt(bob, 0.32))
  }
  await alice.mouse.up()
  await alice.keyboard.press('Escape') // deselected: its resize handle would cover part of the outline
  await sameDrawing()
  const end = (await leftEdgeAt(bob, 0.32))!

  expect(end).toBeGreaterThan(start)
  expect(seen.some((x) => x !== null && x > start && x < end)).toBe(true) // somewhere along the way
})

test("each sees who else is here, and where the other's pointer is", async () => {
  const aliceEmail = (await alice.getByLabel(/\(you\)$/).getAttribute('aria-label'))!.replace(' (you)', '')
  await expect(bob.getByLabel(aliceEmail, { exact: true })).toBeVisible()

  await alice.mouse.move(...(await onDrawing(alice, 0.6, 0.7)))
  await alice.mouse.move(...(await onDrawing(alice, 0.61, 0.7)))
  // Where it came to rest, which arrives up to 100 ms after the first position.
  const pointer = bob.getByRole('img', { name: `${aliceEmail}'s pointer` })
  const [x, y] = await onDrawing(bob, 0.61, 0.7)
  await expect
    .poll(async () => {
      const box = await pointer.boundingBox()
      return box ? Math.hypot(box.x - x, box.y - y) : Infinity
    })
    .toBeLessThan(4)
})

test('undoing is an edit too: the other sees it, and redoing', async () => {
  await alice.getByRole('button', { name: 'Rectangle', exact: true }).click()
  await dragOnDrawing(alice, [0.2, 0.25], [0.3, 0.4])
  const drawn = await sameDrawing()

  await alice.keyboard.press('Control+z')
  expect(await sameDrawing()).toBe(0)
  await alice.keyboard.press('Control+Shift+z')
  expect(await sameDrawing()).toBe(drawn)
})
