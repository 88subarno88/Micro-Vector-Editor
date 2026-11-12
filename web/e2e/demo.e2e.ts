// The demo user, which `make seed` creates, logs in from the login page without a password.
import { expect, test } from '@playwright/test'

test('"Try the demo" logs in as the demo user, who has the sample drawings', async ({ page, request }) => {
  test.skip((await request.get('/api/auth/demo')).status() !== 200, 'no demo here: run `make seed`')

  await page.goto('/')
  await page.getByRole('button', { name: 'Try the demo' }).click()

  await expect(page.getByRole('heading', { name: 'Your documents' })).toBeVisible()
  await expect(page.getByRole('link', { name: 'Welcome to Micro-Vector Editor' })).toBeVisible()
})
