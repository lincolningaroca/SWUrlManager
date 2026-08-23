-- seed.sql corregido
INSERT INTO public.users(user_name,user_password,user_profile,rescue_type,first_value,confirm_value) VALUES('public','public','PUBLIC','PUBLIC','PUBLIC','PUBLIC')
ON CONFLICT (user_name) DO NOTHING;

-- Usuario administrador por defecto — contraseña temporal (cambiar el literal 'Admin123!'
-- antes de distribuir la app si querés un valor distinto). Se fuerza el cambio en el primer login.
INSERT INTO public.users(
  user_name, user_password, user_profile, rescue_type, first_value, confirm_value, user_priv, must_change_password
)
VALUES(
  'admin',
  crypt('Admin123!', gen_salt('bf', 12)),
  'USER',
  'Pin numérico',
  encode(digest('0000'::bytea, 'sha512'), 'hex')::bytea,
  encode(digest('0000'::bytea, 'sha512'), 'hex'),
  'ADMIN',
  true
)
ON CONFLICT (user_name) DO NOTHING;