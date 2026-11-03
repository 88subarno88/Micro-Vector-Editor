-- migrate:up

-- Every token from one login shares a family. Refreshing replaces a token with a new one in the same
-- family; if a replaced token is used again, it has probably been stolen, and the whole family is revoked.
ALTER TABLE refresh_tokens ADD COLUMN family_id UUID;
UPDATE refresh_tokens SET family_id = id;
ALTER TABLE refresh_tokens ALTER COLUMN family_id SET NOT NULL;
CREATE INDEX refresh_tokens_family ON refresh_tokens (family_id);

-- migrate:down
ALTER TABLE refresh_tokens DROP COLUMN family_id;
