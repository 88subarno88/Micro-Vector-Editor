-- migrate:up
CREATE TABLE users (
  id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  email         TEXT UNIQUE NOT NULL,
  password_hash TEXT NOT NULL,  -- argon2id
  created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE documents (
  id         UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  owner_id   UUID NOT NULL REFERENCES users(id),
  title      TEXT NOT NULL,
  head_seq   BIGINT NOT NULL DEFAULT 0,  -- sequence number of the latest applied operation
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE document_members (
  document_id UUID REFERENCES documents(id) ON DELETE CASCADE,
  user_id     UUID REFERENCES users(id) ON DELETE CASCADE,
  role        TEXT NOT NULL CHECK (role IN ('owner', 'editor', 'viewer')),
  PRIMARY KEY (document_id, user_id)
);

-- The primary key can't answer "which documents can this user see?"
CREATE INDEX document_members_user ON document_members (user_id);

-- migrate:down
DROP TABLE document_members, documents, users;
