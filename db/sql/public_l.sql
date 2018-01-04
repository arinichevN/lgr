
CREATE TABLE "peer" (
    "id" TEXT PRIMARY KEY NOT NULL,
    "port" INTEGER NOT NULL,
    "ip_addr" TEXT NOT NULL
);
CREATE TABLE "phone_number" (
    "group_id" INTEGER NOT NULL,
    "value" TEXT NOT NULL
);

