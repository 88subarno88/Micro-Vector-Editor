#!/bin/sh
# Gives a fresh system something to look at: a demo user with the sample drawings in this folder, one
# document each, named after the file. Run by `make seed`, against the API at $API (through nginx). Safe
# to run again: the user is logged in if it exists, and drawings it already has are skipped.
set -eu

api=${API:-http://web}
email=${DEMO_EMAIL:?}
password=${DEMO_PASSWORD:?}
credentials="{\"email\":\"$email\",\"password\":\"$password\"}"

# POSTs JSON; prints the body, and fails unless the status is the one expected.
post() {
    expected=$1
    shift
    status=$(curl -s -o /tmp/body -w '%{http_code}' -X POST -H 'Content-Type: application/json' "$@")
    if [ "$status" != "$expected" ]; then
        echo "seed: $* answered $status: $(cat /tmp/body)" >&2
        exit 1
    fi
    cat /tmp/body
}

# One field of a JSON object, without a JSON tool: the images this runs in have none.
field() { sed -n "s/.*\"$1\":\"\([^\"]*\)\".*/\1/p"; }

status=$(curl -s -o /dev/null -w '%{http_code}' -X POST -H 'Content-Type: application/json' \
    -d "$credentials" "$api/api/auth/signup")
case $status in
    201) echo "seed: created $email" ;;
    409) echo "seed: $email already exists" ;;
    *) echo "seed: signing up answered $status" >&2 && exit 1 ;;
esac
token=$(post 200 -d "$credentials" "$api/api/auth/login" | field accessToken)

existing=$(curl -sf -H "Authorization: Bearer $token" "$api/api/docs?limit=100")
for file in /seed/*.svg; do
    title=$(basename "$file" .svg)
    if echo "$existing" | grep -q "\"title\":\"$title\""; then
        echo "seed: \"$title\" is already there"
        continue
    fi
    curl -sf -o /dev/null -X POST -H "Authorization: Bearer $token" -H 'Content-Type: image/svg+xml' \
        --data-binary "@$file" "$api/api/docs?title=$(echo "$title" | sed 's/ /%20/g')" \
        || { echo "seed: importing \"$title\" failed" >&2 && exit 1; }
    echo "seed: imported \"$title\""
done
echo "seed: done. Log in as $email."
