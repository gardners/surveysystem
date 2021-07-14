import { serializeParams } from './Utils';
import { responseError } from './Api';
import LocalStorage from './storage/LocalStorage';

const {
    REACT_APP_OAUTH2_PROVIDER_URI,
    REACT_APP_OAUTH2_CLIENT_ID,
    REACT_APP_OAUTH2_REDIRECT_URI,
    REACT_APP_OAUTH2_PROVIDER_PROFILE_URI,
} = process.env;

const RESPONSE_TYPE = 'token';

const url = function(path, params) {
  path = path || '';
  params = params || null;

  let q = '';
  if (params) {
    q = '?' + serializeParams(params);
  }

  const base = REACT_APP_OAUTH2_PROVIDER_URI.replace(/\/$/, "");
  return (!path) ? `${base}${q}` : `${base}${path}${q}`;
};

/**
 * generates a random state token
 *
 * @returns {string}
 */
const generate_state = function() {
    return '_' + Math.random().toString(36).substr(2, 9);
};

/**
 * Check if the application is protected by this module
 *
 * @returns {Boolean}
 */
const is_protected = function() {
    return !! REACT_APP_OAUTH2_CLIENT_ID;
}


/**
 * Check if the application is protected by OAthis module
 * @returns {string} access_token
 */
const callback_authorize = function() {

    if (!window.location.hash) {
        return '';
    }

    const hash = window.location.hash.substr(1);
    const urlParams = new URLSearchParams('?' + hash);
    const access_token = urlParams.get('access_token');
    const state = urlParams.get('state');

    let expires_in = urlParams.get('expires_in');

    // validate local token
    const stored_state = LocalStorage.get('ss_state', state);
    if (!stored_state) {
        console.error('Oauth2 redirect callback: no local state stored:');
        return '';
    }

    if (state !== stored_state) {
        console.error('Oauth2 redirect callback: state mismatch:', state, stored_state, state);
        return '';
    }

    // expiry date
    expires_in = parseInt(expires_in, 10);
    if(isNaN(expires_in) || !expires_in) {
        expires_in = 3600;
    }

    let expires_at = new Date().getTime() / 1000;
    expires_at += expires_in - 60;

    LocalStorage.delete('ss_state');
    LocalStorage.set('ss_access_token', access_token);
    LocalStorage.set('ss_expires_at', new Date(expires_at * 1000).toISOString());

    return access_token;
};

/**
 * Verifies a local token
 * @returns {Boolean}
 */
const verify_token = function() {
    if (!is_protected()) {
        return true;
    }

    if (!LocalStorage.get('ss_access_token')) {
        console.log('auth:token:none');
        return false;
    }

    let ms = Date.parse(LocalStorage.get('ss_expires_at'));
    ms = (isNaN(ms)) ? 0 : ms;
    ms -= 6000;

    if(ms <= Date.now()) {
        return false;
    }

    return true;
};

/**
 * construct authorization provider url and optiona lredirect
 */
const exit_authorize = function() {

    // set a local state and store it for authorize_callback
    const state = generate_state();
    LocalStorage.set('ss_state', state);

    const params = {
      client_id: REACT_APP_OAUTH2_CLIENT_ID,
      redirect_uri: REACT_APP_OAUTH2_REDIRECT_URI,
      response_type: RESPONSE_TYPE,
      state: state,
    };

    const uri = url('/authorize', params);
    window.location.href = uri;

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

    if(!verify_token()) {
        exit_authorize();
        return {}; // should never be called
    }

    const access_token = LocalStorage.get('ss_access_token');
    return  {
        Authorization: `Bearer ${access_token}`,
    };
};

/**
 * Fetches user info from provider
 * Returns user object (authenticated) or null
 * @returns {Promise}
 */
const get_user = function() {

    const init =  {
        headers: request_headers(),
    };

    return fetch(REACT_APP_OAUTH2_PROVIDER_PROFILE_URI, init)
        .then((res) => {
            if (!res.ok) {
                if (res.status === 401) {
                    return null;
                }
                return responseError(res);
            }
            return res.json();
        })
        .then(res => Object.assign({
            name: '',
            email: '',
        }, res));
};

/**
 * initialise auth
 * fetch local acces token
 * if it exists, refresh it
 *  TODO: expire use refreshtoken
 * @returns {Promise} string username
 */
const init = function () {

    if (!is_protected()) {
        return Promise.resolve(null);
    }

    // check if local access_token has expired
    if (!verify_token()) {
        return Promise.resolve(null);
    }

    // fetch user object using the local access_token
    // Should the token (surprisingly) be rejected (401) then redirect to authorization endpoint
    return get_user()
        .then((user) => {
            if (!user) {
                exit_authorize();
            }
            return user;
        });
};

const remove_token = function() {
    LocalStorage.delete('ss_access_token');
    LocalStorage.delete('ss_expires_at');
    LocalStorage.delete('ss_state');
}

export {
    is_protected,
    init,
    verify_token,
    remove_token ,
    exit_authorize,
    request_headers,
    callback_authorize,
    get_user
};
