-- seed.sql corregido
INSERT INTO public.users(user_name,user_password,user_profile,rescue_type,first_value,confirm_value) VALUES('public','public','PUBLIC','PUBLIC','PUBLIC','PUBLIC')
ON CONFLICT (user_name) DO NOTHING;
