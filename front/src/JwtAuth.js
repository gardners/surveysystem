import { buildUrl } from './Utils';
import { responseError } from './Api';
import LocalStorage from './storage/LocalStorage';

const {
    REACT_APP_JWT_PROVIDER_URI,
    REACT_APP_JWT_PROVIDER_APPEND_SLASH
} = process.env;

/**
 * Constructs an url , based on REACT_APP_JWT_PROVIDER_URI
 * @returns {string}
 */
const url = function(path, params) {
    let p = path || '';
    if (REACT_APP_JWT_PROVIDER_APPEND_SLASH === 'true') {
        p += '/';
    }
    return buildUrl(REACT_APP_JWT_PROVIDER_URI, p, params);
}

/**
 * Check if the application is protected by this module
 *
 * @returns {Boolean}
 */
const is_protected = function() {
    return !! REACT_APP_JWT_PROVIDER_URI;
}

const add_token = function(name, value) {
    const key = `ss_${name}_token`;
    LocalStorage.set(key, value);
}

const get_token = function(name) {
    const key = `ss_${name}_token`;
    return LocalStorage.get(key);
}

const remove_token = function(name) {
    const key = `ss_${name}_token`;
    LocalStorage.delete(key);
}

const token_exp = function(token) {
    const values = parse_token(token);

    if (!values) {
        return 0;
    }
    return values.exp || 0;
};

/**
 * Get a token from local storage and check if it is expired.
 * Remove an expired token
 */
const get_token_or_expire = function(name) {

    const token = get_token(name);
    if (!token) {
        return null;
    }

    const when = Math.floor(Date.now() / 1000) + (10 * 60); // now + 10 minutes
    const exp = token_exp(token);
    if (exp <= when) {
        remove_token(name);
        return null;
    }

    return token;
}

const parse_token = function(token) {
    try {
        return JSON.parse(atob(token.split('.')[1]));
    } catch (e) {
        return null;
    }
};

/**
 * provides authorization headers for fetch api
 * Should the access token verification fail the function will trigger authorization workflow and exits
 * @returns {object}
 */
const request_headers = function() {
    if(!is_protected()) {
        return {};
    }

    const token = get_token('access');
    if (!token) {
        return {};
    }

    return  {
        Authorization: `Bearer ${token}`,
    };
};

const login = function (username, password) {
    const init =  {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            username,
            password,
        }),
    };

    return fetch(url('/token'), init)
        .then((res) => {
            if (!res.ok) {
                logout();
                return responseError(res);
            }
            return res.json();
        })
        .then((res) => {
            add_token('access', res['access']);
            add_token('refresh', res['refresh']);
            return res['access'];
        });
};

const refresh = function() {
    if (!is_protected()) {
        return Promise.resolve(null);
    }

    const refresh = get_token('refresh');
    if(!refresh) {
        return Promise.resolve(null);
    }

    const init =  {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            refresh,
        }),
    };

    return fetch(url('/token/refresh'), init)
        .then((res) => {
            if (!res.ok) {
                logout();
                return responseError(res);
            }
            return res.json();
        })
        .then((res) => {
            // add tokens (refresh might be optional)
            add_token('access', res['access']);
            if (res['refresh'] !== undefined) {
                add_token('refresh', res['refresh']);
            }
            return res['access'];
        });
};

const get_user = function () {
    if (!is_protected()) {
        return Promise.resolve(null);
    }
    const init = {
        headers: request_headers()
    };

    return fetch(url('/user'), init)
    .then((res) => {
        if (!res.ok) {
            return responseError(res);
        }
        return res.json();
    });
};

/**
 * Init authentication by checking tokens. Refresh tokens if required.
 * Return user if logged in.
 *
 * @returns {Promise}
 */
const init = function () {
    let token = null;

    // not protected (.env)
    if (!is_protected()) {
        return Promise.resolve(null);
    }

    // valid access token?
    token = get_token_or_expire('access');
    if (token) {
        return get_user();
    }

    // valid refresh token?
    token = get_token_or_expire('refresh');
    if (!token) {
        return Promise.resolve(null);
    }

    return refresh().then(() => get_user());
};

const logout = function () {
    remove_token('access');
    remove_token('refresh');
    return Promise.resolve(true);
};

const exports = {
    is_protected,
    init,
    login,
    logout,

    request_headers,
    parse_token,
};

export default exports;
