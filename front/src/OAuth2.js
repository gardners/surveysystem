import cryptoBrowserify from 'crypto-browserify';

import { serializeParams } from './Utils';

const SCOPE = 'read write';
const clientId = process.env.REACT_APP_AUTH_CLIENT_ID;
const redirectUri = process.env.REACT_APP_AUTH_REDIRECT_URI;

const responseError = function(res) {

  return res.text()
  .then((raw) => {

    let message = raw;
    try {
      const json = JSON.parse(raw);
      message = json.error;
    } catch (e) {
      // nothing
    }

    const e = {
      message,
      status: res.status || null,
      statusText: res.statusText || null,
      url: res.url || null,
    };

    throw e;
  });
}

const idToken = function() {
    return  window.localStorage.getItem('ss_id_token');
};

const refreshToken = function() {
    return  window.localStorage.getItem('ss_access_token');
};

const accessToken = function() {
    return  window.localStorage.getItem('ss_access_token');
};

const verifier = function() {
  let res = window.localStorage.getItem('verifier');
  if (!res) {
    res = base64URLEncode(cryptoBrowserify.randomBytes(32));
    window.localStorage.setItem('verifier', res);
  }
  return res;

};

const challenge = function() {
  return base64URLEncode(sha256(verifier()));
};

const url = function(path, params) {
  path = path || '';
  params = params || null;

  let q = '';
  if (params) {
    q = '?' + serializeParams(params);
  }

  const base = process.env.REACT_APP_AUTH_BASE_URL;
  return (!path) ? `${base}/${q}` : `${base}/${path}/${q}`;
}

const base64URLEncode = function(str) {
  return str.toString('base64')
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=/g, '');
};

const sha256 = function (buffer) {
  return cryptoBrowserify.createHash('sha256').update(buffer).digest();
}

const loginUrl = function() {
  return url('authorize', {
    scope: SCOPE,
    response_type: 'code',
    client_id: clientId,
    code_challenge: challenge(),
    code_challenge_method: 'S256',
    redirect_uri: redirectUri
  });
}

const refresh = function(refresh_token) {

  const body = serializeParams({
     grant_type: 'authorization_code',
     client_id: clientId,
     refresh_token,
  });

  return fetch(url('token'), {
      method: 'POST',
      headers: {
        'Content-Type':
        'application/x-www-form-urlencoded'
      },
      body,
  })
  .then((res) => {
    if (!res.ok) {
      return responseError(res); // login?
    }
    return res.json()
  })
  .then((result) => {
    const { access_token, id_token } = result; // TODO refreshtoken updated here?
    window.localStorage.setItem('ss_access_token', access_token);
    window.localStorage.setItem('ss_id_token', id_token);
    window.localStorage.setItem('ss_refresh_token', refresh_token);
  });
};

const login = function(code) {

  const body = serializeParams({
    grant_type: 'authorization_code',
    client_id: clientId,
    code_verifier: verifier(),
    code,
    redirect_uri: redirectUri,
  });

  return fetch(url('token'), {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    body,
  })
  .then((res) => {
    if (!res.ok) {
      return responseError(res);
    }
    return res.json();
  })
  .then((result) => {
    const { access_token, id_token, refresh_token } = result;
    window.localStorage.setItem('ss_access_token', access_token);
    window.localStorage.setItem('ss_id_token', id_token);
    window.localStorage.setItem('ss_refresh_token', refresh_token);
  });
};

const logout = function() {
  const token = accessToken();

  window.localStorage.removeItem('ss_access_token');
  window.localStorage.removeItem('ss_id_token');
  window.localStorage.removeItem('ss_refresh_token');

  return fetch(url('revoke_token'), {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: serializeParams({
        token,
        client_id: clientId,
      }),
  })
  .then((res) => {
    if (!res.ok) {
      return responseError(res); // login?
    }
    return true;
  })
};

const introspect = function(token) {
  return fetch(url('introspect'), {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: serializeParams({
        token,
      }),
  })
  .then((res) => {
    if (!res.ok) {
      return responseError(res); // login?
    }
    return res.json();
  })

}

const getUser = function (access_token) {
    if (!access_token) {
      return refresh()
      .then(() => getUser());
    }

    return fetch(
      url('user'), {
        headers: {
          'Authorization': 'Bearer ' + access_token,
          'Content-Type': 'application/json',
        }
    })
    .then((res) => {
      if (!res.ok) {
        return responseError(res); // login?
      }
      return res.json();
    });
}

export default {
  idToken,
  refreshToken,
  accessToken,
  url,
  loginUrl,

  refresh,
  login,
  logout,
  introspect,

  getUser,
}
