
-- DROP SCHEMA if exists "log" CASCADE;
CREATE SCHEMA "log";

-- DROP TABLE log.v_real;
CREATE TABLE log.v_real
(
  id integer NOT NULL,
  mark timestamp without time zone NOT NULL,
  value real NOT NULL,
  status text NOT NULL,
  CONSTRAINT log_pkey PRIMARY KEY (id, mark)
)
WITH (
  OIDS=FALSE
);

-- DROP TABLE log.alert;
CREATE TABLE log.alert
(
  mark timestamp without time zone NOT NULL,
  message text NOT NULL
)
WITH (
  OIDS=FALSE
);


-- DROP FUNCTION log.do_real(integer, real, integer, integer, text);

CREATE OR REPLACE FUNCTION log.do_real(
    in_id integer,
    val real,
    row_limit integer,
    tm integer,
    sts text)
  RETURNS integer AS
$BODY$declare
 n bigint;
begin
  select count(*) from log.v_real where id=in_id into n;
  if not FOUND then
    raise exception 'count failed when id was: % ', in_id;
  end if;
  if n<row_limit then
    insert into log.v_real(id, mark, value, status) values (in_id, to_timestamp(tm), val, sts);
    if not FOUND then
      raise exception 'insert failed when id was: % ', in_id;
    end if;
    return 1;
  else
   update log.v_real set mark=to_timestamp(tm), value=val, status=sts where id=in_id and mark=(select min(mark) from log.v_real where id=in_id);
   if not FOUND then
     raise exception 'update failed when id was: % ', in_id;
   end if;
   return 2;
  end if;
 return 0;
end;$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;