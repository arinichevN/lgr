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
