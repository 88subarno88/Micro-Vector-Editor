-- migrate:up

-- Every edit to a document, in order. The evolved CommandStack.
CREATE TABLE operations (
  document_id  UUID   NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
  seq          BIGINT NOT NULL,
  user_id      UUID   NOT NULL REFERENCES users(id),
  client_op_id TEXT   NOT NULL,  -- sent by the client, so that a retry can't apply twice
  payload      JSONB  NOT NULL,  -- a vec_core Operation
  created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (document_id, seq),
  UNIQUE (document_id, client_op_id)
);

-- Periodic full copies, so that loading a document doesn't replay its whole log.
CREATE TABLE snapshots (
  document_id UUID   NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
  seq         BIGINT NOT NULL,  -- the last operation included
  state       JSONB  NOT NULL,  -- a vec_core Diagram
  created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (document_id, seq)
);

-- migrate:down
DROP TABLE snapshots, operations;
