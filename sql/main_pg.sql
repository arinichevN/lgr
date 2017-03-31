
DROP SCHEMA if exists lgr CASCADE;
CREATE SCHEMA lgr;


CREATE TABLE lgr.config
(
  app_class character varying(32) NOT NULL,
  db_public character varying(256) NOT NULL,
  udp_port character varying(32) NOT NULL,
  pid_path character varying(32) NOT NULL,
  udp_buf_size character varying(32) NOT NULL,
  db_data character varying(32) NOT NULL,
  db_log character varying(32) NOT NULL,
  cycle_duration_us character varying(32) NOT NULL,
  CONSTRAINT config_pkey PRIMARY KEY (app_class)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE lgr.prog
(
  app_class character varying(32) NOT NULL,
  id integer NOT NULL,
  description character varying(32) NOT NULL,
  sensor_fts_id character varying(32) NOT NULL,
  kind character varying(32) NOT NULL,
  interval_min interval NOT NULL,
  max_rows integer NOT NULL,
  active integer NOT NULL,
  CONSTRAINT item_pkey PRIMARY KEY (app_class, id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE lgr.sensor_mapping
(
  app_class character varying(32) NOT NULL,
  sensor_id integer NOT NULL,
  peer_id character varying(32) NOT NULL,
  remote_id integer NOT NULL,
  CONSTRAINT sensor_mapping_pkey PRIMARY KEY (app_class, sensor_id, peer_id, remote_id)
)
WITH (
  OIDS=FALSE
);



