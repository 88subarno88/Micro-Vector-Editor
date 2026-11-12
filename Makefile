# The whole system in containers (deploy/docker-compose.yml), with its settings from deploy/.env.
COMPOSE = docker compose -f deploy/docker-compose.yml --env-file deploy/.env

.PHONY: up down logs seed

# Builds the images and starts everything, returning once every service is healthy.
up: deploy/.env
	$(COMPOSE) up --build --detach --wait

# Stops everything. The database and exports stay, in their volumes; `docker compose down -v` removes them.
down:
	$(COMPOSE) down

logs:
	$(COMPOSE) logs --follow

# A demo user with the sample drawings (deploy/seed). Safe to run again.
seed: deploy/.env
	$(COMPOSE) run --rm seed

deploy/.env:
	@echo "deploy/.env is missing: copy deploy/.env.example and fill it in." && false
