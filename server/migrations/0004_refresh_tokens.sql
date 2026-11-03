-- migrate:up

-- Long-lived login sessions. Only a hash of each token is stored, so a leaked table can't be used to
-- log in.
CREATE TABLE refresh_tokens (
  id         UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id    UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  token_hash TEXT NOT NULL UNIQUE,  -- SHA-256 of the token, in hex
  expires_at TIMESTAMPTZ NOT NULL,
  revoked_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX refresh_tokens_user ON refresh_tokens (user_id);

-- migrate:down
DROP TABLE refresh_tokens;
