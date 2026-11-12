# The whole system in containers (deploy/docker-compose.yml), with its settings from deploy/.env.
COMPOSE = docker compose -f deploy/docker-compose.yml --env-file deploy/.env

.PHONY: up down logs seed share

# Builds the images and starts everything, returning once every service is healthy.
up: deploy/.env
	$(COMPOSE) up --build --detach --wait

# Stops everything, the tunnel included. The database and exports stay, in their volumes;
# `docker compose down -v` removes them.
down:
	$(COMPOSE) --profile share down

logs:
	$(COMPOSE) logs --follow

# Puts the app on the internet, at a new https://….trycloudflare.com address, until `make down`. Only while
# this machine is on.
share: up
	$(COMPOSE) --profile share up --detach tunnel
	@for i in $$(seq 30); do \
		url=$$($(COMPOSE) logs tunnel 2>/dev/null | grep -o 'https://[a-z0-9-]*\.trycloudflare\.com' | tail -1); \
		[ -n "$$url" ] && echo "The app is on $$url" && exit 0; sleep 1; \
	done; echo "No address yet: see make logs." && false

# A demo user with the sample drawings (deploy/seed). Safe to run again.
seed: deploy/.env
	$(COMPOSE) run --rm seed

deploy/.env:
	@echo "deploy/.env is missing: copy deploy/.env.example and fill it in." && false
