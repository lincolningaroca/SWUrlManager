-- =============================================================================
-- SCRIPT DE INICIALIZACIÓN AUTOMÁTICA
-- Habilita pgcrypto, crea esquemas, tablas, índices, llaves y funciones PL/pgSQL.
-- =============================================================================

-- 1. EXTENSIÓN DE ENCRIPTACIÓN
CREATE EXTENSION IF NOT EXISTS pgcrypto WITH SCHEMA public;

-- 2. CREACIÓN DE ESQUEMAS Y TABLAS
CREATE SCHEMA IF NOT EXISTS sys_audit;

CREATE TABLE IF NOT EXISTS public.users (
    user_id integer NOT NULL GENERATED ALWAYS AS IDENTITY,
    user_name text NOT NULL,
    user_password text NOT NULL,
    user_profile character varying(20) NOT NULL,
    rescue_type character varying(40) NOT NULL,
    first_value bytea NOT NULL,
    confirm_value text NOT NULL,
    user_priv character varying(10) NOT NULL DEFAULT 'USER',
    is_active boolean NOT NULL DEFAULT true,
    can_edit boolean NOT NULL DEFAULT true,
    CONSTRAINT pk_user PRIMARY KEY (user_id),
    CONSTRAINT uq_user_name UNIQUE (user_name),
    CONSTRAINT chk_user_priv CHECK (user_priv IN ('ADMIN', 'USER'))
);
ALTER TABLE public.users
  ADD COLUMN IF NOT EXISTS must_change_password boolean NOT NULL DEFAULT false;

CREATE TABLE IF NOT EXISTS public.category (
    category_id integer NOT NULL GENERATED ALWAYS AS IDENTITY,
    category_name text NOT NULL,
    category_desc text,
    userid integer NOT NULL,
    CONSTRAINT pk_category PRIMARY KEY (category_id),
    CONSTRAINT fk_user_category FOREIGN KEY (userid) REFERENCES public.users(user_id)
);

CREATE TABLE IF NOT EXISTS public.urls (
    url_id integer NOT NULL GENERATED ALWAYS AS IDENTITY,
    url_text bytea NOT NULL,
    url_desc bytea,
    categoryid integer NOT NULL,
    url_hash text,
    CONSTRAINT pk_url PRIMARY KEY (url_id),
    CONSTRAINT uq_url UNIQUE (url_text),
    CONSTRAINT fk_category_urls FOREIGN KEY (categoryid) REFERENCES public.category(category_id)
);

-- Tabla de Auditoría (Oculta en esquema sys_audit)
CREATE TABLE IF NOT EXISTS sys_audit.user_sessions (
    session_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY,
    user_id integer NOT NULL,
    login_at timestamp with time zone NOT NULL DEFAULT CURRENT_TIMESTAMP,
    logout_at timestamp with time zone,
    client_info text,
    details jsonb DEFAULT '{}'::jsonb,
    CONSTRAINT pk_user_sessions PRIMARY KEY (session_id),
    CONSTRAINT fk_user_sessions_users FOREIGN KEY (user_id) REFERENCES public.users(user_id) ON DELETE CASCADE
);

-- 3. ÍNDICES
CREATE INDEX IF NOT EXISTS idx_url_hash ON public.urls USING btree (url_hash);

-- 4. FUNCIONES ALMACENADAS (PL/pgSQL)

CREATE OR REPLACE FUNCTION public.fn_update_url_by_text(
    p_url text,
    p_desc text,
    p_categoryid integer,
    p_key text
) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    UPDATE urls
    SET url_desc = pgp_sym_encrypt(p_desc, p_key)
    WHERE url_hash = encode(digest(TRIM(p_url)::BYTEA, 'sha256'), 'hex')
      AND categoryid = p_categoryid;

    RETURN FOUND;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_any_user_exists() RETURNS boolean
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_count INTEGER;
BEGIN
  SELECT COUNT(*) INTO v_count
  FROM users
  WHERE user_name <> 'public';
  RETURN (v_count > 0);
END;
$$;

-- Funciones de Auditoría de Sesiones en su propio esquema
CREATE OR REPLACE FUNCTION sys_audit.fn_audit_login(p_user_id integer, p_client_info text DEFAULT NULL::text, p_details jsonb DEFAULT '{}'::jsonb) RETURNS bigint
    LANGUAGE plpgsql SECURITY DEFINER
    AS $$
DECLARE
  v_session_id BIGINT;
BEGIN
  INSERT INTO sys_audit.user_sessions (user_id, client_info, details)
  VALUES (p_user_id, p_client_info, p_details)
  RETURNING session_id INTO v_session_id;

  RETURN v_session_id;
END;
$$;

CREATE OR REPLACE FUNCTION sys_audit.fn_audit_logout(p_session_id bigint) RETURNS void
    LANGUAGE plpgsql SECURITY DEFINER
    AS $$
BEGIN
  UPDATE sys_audit.user_sessions
  SET logout_at = CURRENT_TIMESTAMP
  WHERE session_id = p_session_id AND logout_at IS NULL;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_category_exists(p_name text, p_userid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_count INTEGER;
BEGIN
  SELECT COUNT(*) INTO v_count
  FROM category
  WHERE category_name = p_name
  AND   userid        = p_userid;
  RETURN (v_count > 0);
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_create_user(p_username text, p_password text, p_profile text, p_rescue_type text, p_first_value text, p_confirm_value text, p_key text)
 RETURNS boolean
 LANGUAGE plpgsql
AS $function$
BEGIN
  INSERT INTO users(
    user_name,
    user_password,
    user_profile,
    rescue_type,
    first_value,
    confirm_value,
    user_priv
  )
  VALUES(
    p_username,
    crypt(p_password, gen_salt('bf', 12)),
    p_profile,
    p_rescue_type,
    CASE
      WHEN p_rescue_type = 'Pin numérico'
        THEN encode(digest(p_first_value::BYTEA, 'sha512'), 'hex')::BYTEA
      ELSE
        pgp_sym_encrypt(p_first_value, p_key)
    END,
    encode(digest(p_confirm_value::BYTEA, 'sha512'), 'hex'),
    'USER'
  );
  RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
  RETURN FALSE;
END;
$function$
;

CREATE OR REPLACE FUNCTION public.fn_delete_category(p_category_id integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    DELETE FROM category WHERE category_id = p_category_id;
    RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_delete_url_by_id(p_url_id integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    DELETE FROM urls WHERE url_id = p_url_id;
    RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_delete_urls_by_category(p_categoryid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    DELETE FROM urls WHERE categoryid = p_categoryid;
    RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_category_data(p_category_id integer) RETURNS TABLE(cat_name text, cat_desc text)
    LANGUAGE plpgsql
    AS $$
BEGIN
    RETURN QUERY
    SELECT category_name, COALESCE(category_desc, '')
    FROM category
    WHERE category_id = p_category_id;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_question(p_userid integer, p_key text) RETURNS text
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_rescue_type TEXT;
  v_first_value BYTEA;
BEGIN
  SELECT rescue_type, first_value
  INTO v_rescue_type, v_first_value
  FROM users
  WHERE user_id = p_userid;

  IF v_rescue_type = 'Pin numérico' THEN
    RETURN NULL;
  END IF;

  RETURN pgp_sym_decrypt(v_first_value, p_key);
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_rescue_data(p_userid integer, p_key text) RETURNS TABLE(rescue_type text, first_value text)
    LANGUAGE plpgsql
    AS $$
BEGIN
  RETURN QUERY
  SELECT
    u.rescue_type,
    CASE
      WHEN u.rescue_type = 'Pin numérico'
        THEN NULL
      ELSE
        pgp_sym_decrypt(u.first_value, p_key)
    END
  FROM users u
  WHERE u.user_id = p_userid;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_urls(p_categoryid integer, p_key text) RETURNS TABLE(url_id integer, url_text text, url_desc text)
    LANGUAGE plpgsql
    AS $$
BEGIN
  RETURN QUERY
  SELECT
    u.url_id,
    pgp_sym_decrypt(u.url_text, p_key),
    pgp_sym_decrypt(u.url_desc, p_key)
  FROM urls u
  WHERE u.categoryid = p_categoryid;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_urls_by_id(p_url_id integer, p_key text) RETURNS TABLE(url_id integer, url_text text, url_desc text)
    LANGUAGE plpgsql
    AS $$
BEGIN
  RETURN QUERY
  SELECT
    u.url_id,
    pgp_sym_decrypt(u.url_text, p_key),
    pgp_sym_decrypt(u.url_desc, p_key)
  FROM urls u
  WHERE u.url_id = p_url_id;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_get_user_id(p_username text, p_profile text)
 RETURNS integer
 LANGUAGE plpgsql
AS $function$
DECLARE
  v_id INTEGER;
BEGIN
  SELECT user_id INTO v_id
  FROM users
  WHERE user_name    = p_username
  AND   user_profile = p_profile;
  RETURN COALESCE(v_id, 0);
END;
$function$
;

CREATE OR REPLACE FUNCTION public.fn_load_category_list(p_userid integer) RETURNS TABLE(cat_id integer, cat_name text)
    LANGUAGE plpgsql
    AS $$
BEGIN
    RETURN QUERY
    SELECT category_id, category_name
    FROM category
    WHERE userid = p_userid
    ORDER BY category_name ASC;

END;
$$;

CREATE OR REPLACE FUNCTION public.fn_login(p_username text, p_password text)
 RETURNS boolean
 LANGUAGE plpgsql
AS $function$
DECLARE
  v_stored_password TEXT;
  v_is_active BOOLEAN;
BEGIN
  SELECT user_password, is_active INTO v_stored_password, v_is_active
  FROM users
  WHERE user_name = p_username;

  IF NOT FOUND OR NOT v_is_active THEN
    RETURN FALSE;
  END IF;

  RETURN (crypt(p_password, v_stored_password) = v_stored_password);
END;
$function$
;

CREATE OR REPLACE FUNCTION public.fn_get_user_permissions(p_username text)
RETURNS TABLE(user_priv text, can_edit boolean, must_change_password boolean)
    LANGUAGE plpgsql
    AS $$
BEGIN
  RETURN QUERY
  SELECT u.user_priv::text, u.can_edit, u.must_change_password
  FROM users u
  WHERE u.user_name = p_username;
END;
$$;


CREATE OR REPLACE FUNCTION public.fn_move_url_to_category(p_url_id integer, p_new_categoryid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    UPDATE urls
    SET categoryid = p_new_categoryid
    WHERE url_id = p_url_id;

    RETURN FOUND;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_reset_password(p_userid integer, p_password text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
  UPDATE users
  SET user_password = crypt(p_password, gen_salt('bf', 12)),
      must_change_password = false
  WHERE user_id = p_userid;
  RETURN FOUND;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_save_category(p_name text, p_desc text, p_userid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
  INSERT INTO category(category_name, category_desc, userid)
  VALUES(
    UPPER(TRIM(p_name)),
    UPPER(TRIM(p_desc)),
    p_userid
  );
  RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
  RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_save_url(p_url text, p_desc text, p_categoryid integer, p_key text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
  INSERT INTO urls(url_text, url_desc, url_hash, categoryid)
  VALUES(
    pgp_sym_encrypt(TRIM(p_url), p_key),
    pgp_sym_encrypt(p_desc,      p_key),
    encode(digest(TRIM(p_url)::BYTEA, 'sha256'), 'hex'),
    p_categoryid
  );
  RETURN TRUE;
EXCEPTION WHEN OTHERS THEN
  RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_update_category(p_name text, p_desc text, p_category_id integer, p_userid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    UPDATE category
    SET category_name = UPPER(TRIM(p_name)),
        category_desc = UPPER(TRIM(p_desc))
    WHERE category_id = p_category_id AND userid = p_userid;

    RETURN FOUND;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_update_url(p_url text, p_desc text, p_url_id integer, p_categoryid integer, p_key text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
    UPDATE urls
    SET
        url_text = pgp_sym_encrypt(TRIM(p_url), p_key),
        url_desc = pgp_sym_encrypt(p_desc, p_key),
        url_hash = encode(digest(TRIM(p_url)::BYTEA, 'sha256'), 'hex')
    WHERE
        url_id = p_url_id
        AND categoryid = p_categoryid;

    RETURN FOUND;
EXCEPTION WHEN OTHERS THEN
    RAISE EXCEPTION '%', SQLERRM;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_url_exists(p_url text, p_categoryid integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_count INTEGER;
BEGIN
  SELECT COUNT(*) INTO v_count
  FROM urls
  WHERE url_hash   = encode(digest(TRIM(p_url)::BYTEA, 'sha256'), 'hex')
  AND   categoryid = p_categoryid;
  RETURN (v_count > 0);
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_user_exists(p_username text)
 RETURNS boolean
 LANGUAGE plpgsql
AS $function$
DECLARE
  v_count INTEGER;
BEGIN
  SELECT COUNT(*) INTO v_count
  FROM users
  WHERE user_name = p_username;
  RETURN (v_count > 0);
END;
$function$
;


CREATE OR REPLACE FUNCTION public.fn_validate_answer(p_userid integer, p_answer text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_count INTEGER;
BEGIN
  SELECT COUNT(*) INTO v_count
  FROM users
  WHERE user_id       = p_userid
  AND   confirm_value = encode(digest(p_answer::BYTEA, 'sha512'), 'hex');
  RETURN (v_count > 0);
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_validate_rescue_type(p_userid integer) RETURNS text
    LANGUAGE plpgsql
    AS $$
DECLARE
  v_type TEXT;
BEGIN
  SELECT rescue_type INTO v_type
  FROM users
  WHERE user_id = p_userid;
  RETURN v_type;
END;
$$;

CREATE OR REPLACE FUNCTION public.fn_export_user_data(p_userid integer, p_key text)
RETURNS TABLE(category_name text, category_desc text, url_text text, url_desc text)
    LANGUAGE plpgsql
    AS $$
BEGIN
  RETURN QUERY
  SELECT c.category_name, COALESCE(c.category_desc, ''),
         pgp_sym_decrypt(u.url_text, p_key),
         pgp_sym_decrypt(u.url_desc, p_key)
  FROM category c
  JOIN urls u ON u.categoryid = c.category_id
  WHERE c.userid = p_userid
  ORDER BY c.category_name, u.url_id;
END;
$$;

CREATE TABLE IF NOT EXISTS public.security_settings (
    id              smallint NOT NULL DEFAULT 1,
    kdf_salt        bytea    NOT NULL,
    dek_nonce       bytea    NOT NULL,
    dek_tag         bytea    NOT NULL,
    wrapped_dek     bytea    NOT NULL,
    kdf_iterations  integer  NOT NULL,
    schema_version  smallint NOT NULL,
    updated_at      timestamp with time zone NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT pk_security_settings PRIMARY KEY (id),
    CONSTRAINT chk_security_settings_single_row CHECK (id = 1)
);

CREATE OR REPLACE FUNCTION public.fn_bootstrap_admin(p_password text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
BEGIN
  IF EXISTS (SELECT 1 FROM users WHERE user_priv = 'ADMIN') THEN
    RETURN FALSE;
  END IF;

  INSERT INTO users(
    user_name, user_password, user_profile, rescue_type, first_value, confirm_value, user_priv, must_change_password
  )
  VALUES(
    'admin',
    crypt(p_password, gen_salt('bf', 12)),
    'USER',
    'Pin numérico',
    encode(digest('0000'::bytea, 'sha512'), 'hex')::bytea,
    encode(digest('0000'::bytea, 'sha512'), 'hex'),
    'ADMIN',
    true
  )
  ON CONFLICT (user_name) DO NOTHING;

  RETURN TRUE;
END;
$$;