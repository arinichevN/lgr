
CREATE TABLE "prog" (
    "id" INTEGER PRIMARY KEY,
    "description" TEXT NOT NULL,
    "sensor_fts_id" INTEGER NOT NULL,
    "kind" TEXT NOT NULL,
    "interval_min" INTEGER NOT NULL,
    "max_rows" INTEGER NOT NULL,
    "enable" INTEGER NOT NULL,
    "load" INTEGER NOT NULL
);


CREATE TABLE "sensor_mapping" (
    "sensor_id" INTEGER PRIMARY KEY NOT NULL,
    "peer_id" TEXT NOT NULL,
    "remote_id" INTEGER NOT NULL
);



