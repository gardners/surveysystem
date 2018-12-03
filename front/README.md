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

Css themes are located in the `styles` directories and extend bootstrap SASS.
Some example themes are included from [https://bootswatch.com](https://bootswatch.com)

Build App with a theme.

```
#start
REACT_APP_THEME=<name> npm start
# example: REACT_APP_THEME=slate npm start

#build
REACT_APP_THEME=<name> npm build
# example: REACT_APP_THEME=slate npm build
```
Where the name of the theme is simply a folder name inside the `styles` dir

Development only:

add `?theme=slate` to the browser url and refresh

---

# Default usage

## Available Scripts

In the project directory, you can run:

### `npm start`

Runs the app in the development mode.<br>
Open [http://localhost:3000](http://localhost:3000) to view it in the browser.

The page will reload if you make edits.<br>
You will also see any lint errors in the console.

### `npm test`

Launches the test runner in the interactive watch mode.<br>
See the section about [running tests](https://facebook.github.io/create-react-app/docs/running-tests) for more information.

### `npm run build`

Builds the app for production to the `build` folder.<br>
It correctly bundles React in production mode and optimizes the build for the best performance.

The build is minified and the filenames include the hashes.<br>
Your app is ready to be deployed!

See the section about [deployment](https://facebook.github.io/create-react-app/docs/deployment) for more information.

### `npm run eject`

**Note: this is a one-way operation. Once you `eject`, you can’t go back!**

If you aren’t satisfied with the build tool and configuration choices, you can `eject` at any time. This command will remove the single build dependency from your project.

Instead, it will copy all the configuration files and the transitive dependencies (Webpack, Babel, ESLint, etc) right into your project so you have full control over them. All of the commands except `eject` will still work, but they will point to the copied scripts so you can tweak them. At this point you’re on your own.

You don’t have to ever use `eject`. The curated feature set is suitable for small and middle deployments, and you shouldn’t feel obligated to use this feature. However we understand that this tool wouldn’t be useful if you couldn’t customize it when you are ready for it.

## Learn More

You can learn more in the [Create React App documentation](https://facebook.github.io/create-react-app/docs/getting-started).

To learn React, check out the [React documentation](https://reactjs.org/).
