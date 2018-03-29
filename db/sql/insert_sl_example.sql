delete from "peer";
INSERT INTO "peer" VALUES('gwu18_1',49161,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu22_1',49162,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu18_2',49161,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu22_2',49162,'127.0.0.1');
INSERT INTO "peer" VALUES('regonf_1',49191,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu74_1',49163,'127.0.0.1');
INSERT INTO "peer" VALUES('lck_1',49175,'127.0.0.1');
INSERT INTO "peer" VALUES('lgr_1',49172,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu59_1',49164,'127.0.0.1');
INSERT INTO "peer" VALUES('alp_1',49171,'127.0.0.1');
INSERT INTO "peer" VALUES('alr_1',49174,'127.0.0.1');
INSERT INTO "peer" VALUES('regsmp_1',49192,'127.0.0.1');
INSERT INTO "peer" VALUES('stp_1',49179,'127.0.0.1');
INSERT INTO "peer" VALUES('obj_1',49178,'127.0.0.1');
INSERT INTO "peer" VALUES('swr_1',49183,'127.0.0.1');
INSERT INTO "peer" VALUES('swf_1',49182,'127.0.0.1');

delete from sensor_mapping;
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (1, 'obj_1', 1);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (2, 'obj_1', 2);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (3, 'obj_1', 3);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (4, 'obj_1', 4);

delete from prog;
INSERT INTO prog (id,description, sensor_fts_id, kind, interval_min, max_rows, enable,load)
VALUES (1, 'канал1', 1, 'fts', 60, 720,1,1);

INSERT INTO prog (id,description, sensor_fts_id, kind, interval_min, max_rows, enable,load)
VALUES (2, 'канал2', 2, 'fts', 60, 720, 1,1);

INSERT INTO prog (id,description, sensor_fts_id, kind, interval_min, max_rows, enable,load)
VALUES (3, 'канал3', 3, 'fts', 60, 720, 1,1);

INSERT INTO prog (id,description, sensor_fts_id, kind, interval_min, max_rows, enable,load)
VALUES (4, 'канал4', 4, 'fts', 60, 720, 1,1);
