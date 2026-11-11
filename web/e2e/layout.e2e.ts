// The drawing must stay put on the page: anything that moved it would make the next click land somewhere
// other than where the user aimed.
import { expect, test } from '@playwright/test'
import { dragOnDrawing, newUser, onDrawing, openAs, sharedDocument } from './helpers'

test("the drawing doesn't move when someone joins, a shape is selected, or the connection changes", async ({
  browser,
  request,
}) => {
  const [alice, bob] = [await newUser(request, 'alice'), await newUser(request, 'bob')]
  await sharedDocument(request, alice, [bob], 'Steady')
  const page = await openAs(browser, alice, 'Steady')
  const drawingTop = async () => (await page.getByRole('img', { name: 'The drawing' }).boundingBox())!.y
  const before = await drawingTop()

  await openAs(browser, bob, 'Steady') // a second avatar appears
  await expect(page.getByLabel(bob.email, { exact: true })).toBeVisible()
  expect(await drawingTop()).toBe(before)

  await page.getByRole('button', { name: 'Rectangle', exact: true }).click()
  await dragOnDrawing(page, [0.2, 0.25], [0.3, 0.4])
  await page.getByRole('button', { name: 'Select', exact: true }).click()
  await page.mouse.click(...(await onDrawing(page, 0.25, 0.32))) // the style panel shows
  await expect(page.getByLabel('Stroke')).toBeVisible()
  expect(await drawingTop()).toBe(before)

  await page.context().setOffline(true) // the status changes
  await expect(page.getByRole('status')).toHaveText('Offline')
  expect(await drawingTop()).toBe(before)
  await page.context().setOffline(false)
})
