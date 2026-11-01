-- migrate:up

-- Export jobs double as the job queue: workers claim rows with FOR UPDATE SKIP LOCKED.
CREATE TABLE export_jobs (
  id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  document_id UUID NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
  format      TEXT NOT NULL CHECK (format IN ('svg', 'png')),
  status      TEXT NOT NULL CHECK (status IN ('queued', 'running', 'done', 'failed')),
  attempts    INT  NOT NULL DEFAULT 0,
  run_after   TIMESTAMPTZ NOT NULL DEFAULT now(),
  result_url  TEXT,
  error       TEXT,
  created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX export_jobs_queue ON export_jobs (status, run_after);

-- migrate:down
DROP TABLE export_jobs;
