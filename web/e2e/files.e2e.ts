// Importing an SVG file and downloading exports, which the export worker makes.
import { readFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'
import { expect, test } from '@playwright/test'
import { logIn, newUser } from './helpers'

const fixtures = fileURLToPath(new URL('../../core/tests/fixtures/', import.meta.url))

test('an imported SVG file becomes a document, which downloads as SVG and as PNG', async ({ page, request }) => {
  await logIn(page, await newUser(request, 'importer'))

  await page.getByLabel('Import SVG').setInputFiles(`${fixtures}malformed.svg`)
  await expect(page.getByRole('alert')).toHaveText(/couldn't be read as SVG \(line 6, column \d+/)

  await page.getByLabel('Import SVG').setInputFiles(`${fixtures}app-export.svg`)
  await expect(page.getByRole('heading', { name: 'app-export' })).toBeVisible()

  const download = async (name: string) => {
    const [file] = await Promise.all([page.waitForEvent('download'), page.getByRole('button', { name }).click()])
    return { name: file.suggestedFilename(), bytes: readFileSync(await file.path()) }
  }
  const svg = await download('Download SVG')
  expect(svg.name).toBe('app-export.svg')
  expect(svg.bytes.toString()).toContain('<svg')
  const png = await download('Download PNG')
  expect(png.name).toBe('app-export.png')
  expect(png.bytes.subarray(0, 8).toString('hex')).toBe('89504e470d0a1a0a') // the PNG signature
})
