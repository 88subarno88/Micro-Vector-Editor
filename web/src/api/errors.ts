import type { ApiError } from './client'

const messages: Record<string, string> = {
  invalid_credentials: 'Wrong email or password.',
  invalid_email: 'Enter a valid email address.',
  invalid_password: 'Use a password of 8 to 1024 characters.',
  email_taken: 'An account with that email already exists. Log in instead.',
  invalid_title: 'Use a title of 1 to 200 characters, on one line.',
  forbidden: "You don't have permission to do that here.",
  not_found: "That document doesn't exist, or you don't have access to it.",
}

// What to tell the user about a failed request: a plain message for the codes we know, the server's own
// message otherwise.
export function describeFailure(error: ApiError | undefined, response: Response | undefined): string {
  if (!response) return "Can't reach the server. Check your connection and try again."
  if (response.status === 429) {
    const seconds = Number(response.headers.get('Retry-After'))
    return seconds > 0 ? `Too many attempts. Try again in ${seconds} seconds.` : 'Too many attempts. Try again soon.'
  }
  return (error && messages[error.error.code]) ?? error?.error.message ?? 'Something went wrong. Try again.'
}

// Runs an api call, turning a network failure into a result with no response instead of a throw, so that
// callers handle every failure in one place with describeFailure.
export async function attempt<T>(
  request: () => Promise<T>,
): Promise<T | { data: undefined; error: undefined; response: undefined }> {
  try {
    return await request()
  } catch {
    return { data: undefined, error: undefined, response: undefined }
  }
}
