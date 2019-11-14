This project was bootstrapped with [Create React App](https://github.com/facebook/create-react-app).

# Custom usage

## custom scripts

### `npm run dev`

Runs the app **and** starts *backend server* (lighthttpd) in an external terminal instance (PORT: see your `lighttpd.conf.local` in `../backend` folder)

### `npm run dev-mock`

Runs the app **and** starts *mockserver* (node-http) in an external terminal instance (PORT: `3099`)

### troubleshooting dev servers

* mocksever exits with `EADDRINUSE`

```bash
sudo kill $(sudo lsof -t -i:<YOUR PORT>)
```

* backend server: CORS/Network error in browser console

```bash
# check status
sudo service lighttpd status
# check systemd logs
sudo  sudo journalctl -u lighttpd
```

In most cases your port is being used by another service!
Let's say you configured the backend on port 80, but have `nginx` running on port 80 already. Either change port or stop (or disable) the nginx service.

```bash
sudo lsof  -i:80
# Ah! nginx processes listed!
sudo service nginx stop
sudo service lighttpd start
```

# Css Themes

Css themes are located in the `styles` directories and extend Bootstrap SASS.

Creating a theme:

```bash
cp ./src/styles/default ./src/styles/<THEME-NAME>
```

Themes can be set permanently in your `.env.<ENVIRONMENT>.local` using `REACT_APP_THEME="<THEME-NAME>"` global variable.

Ad hoc theme assignment

```
REACT_APP_THEME=<THEME-NAME> npm start
```
