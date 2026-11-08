import { beforeEach, describe, expect, it, vi } from 'vitest'

const createVecModule = vi.fn()
vi.mock('../wasm/vec.js', () => ({ default: createVecModule }))

// A fresh copy of the module each time, since it remembers what it loaded.
const freshLoadCore = async () => (await import('./core')).loadCore

beforeEach(() => {
  vi.resetModules()
  createVecModule.mockReset()
})

describe('loadCore', () => {
  it('loads the module once, however many callers ask', async () => {
    const module = { Document: class {} }
    createVecModule.mockResolvedValue(module)
    const loadCore = await freshLoadCore()

    const results = await Promise.all([loadCore(), loadCore(), loadCore()])

    expect(createVecModule).toHaveBeenCalledOnce()
    expect(results.every((result) => result === module)).toBe(true)
    expect(await loadCore()).toBe(module)
    expect(createVecModule).toHaveBeenCalledOnce()
  })

  it('tries again after a failure', async () => {
    const module = { Document: class {} }
    createVecModule.mockRejectedValueOnce(new Error('offline')).mockResolvedValueOnce(module)
    const loadCore = await freshLoadCore()

    await expect(loadCore()).rejects.toThrow('offline')
    expect(await loadCore()).toBe(module)
    expect(createVecModule).toHaveBeenCalledTimes(2)
  })
})
