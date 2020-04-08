-- SCHEMA: measurements

-- DROP SCHEMA measurements ;

CREATE SCHEMA measurements
    AUTHORIZATION postgres;

-- Table: measurements.deployment

-- DROP TABLE measurements.deployment;

CREATE TABLE measurements.deployment
(
    id integer NOT NULL DEFAULT nextval('measurements.deployment_id_seq'::regclass),
    "IMEI" text COLLATE pg_catalog."default" NOT NULL,
    location geometry NOT NULL,
    height numeric NOT NULL,
    deployed timestamp with time zone NOT NULL,
    retreived timestamp with time zone,
    CONSTRAINT deployment_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE measurements.deployment
    OWNER to postgres;

-- Table: measurements.measurement

-- DROP TABLE measurements.measurement;

CREATE TABLE measurements.measurement
(
    deployment integer NOT NULL,
    "time" timestamp with time zone NOT NULL,
    level numeric NOT NULL,
    water_temp numeric,
    air_temp numeric,
    humidity numeric,
    pressure numeric,
    CONSTRAINT measurement_pkey PRIMARY KEY (deployment),
    CONSTRAINT deployment FOREIGN KEY (deployment)
        REFERENCES measurements.deployment (id) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
)

TABLESPACE pg_default;

ALTER TABLE measurements.measurement
    OWNER to postgres;