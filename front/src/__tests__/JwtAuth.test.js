import LocalStorage from '../storage/LocalStorage';
import Auth from '../JwtAuth';


const MOCK_ENDPOINT = 'https://test.me';
const ONE_HOUR = 3600; //seconds

// store original fetch api
const _cached_fetch = window.fetch;

const mockFetchApi = function(resource, options={}) {
    let data = null;

    switch (resource) {
        case `${MOCK_ENDPOINT}/token`:
        case `${MOCK_ENDPOINT}/token/refresh`:
            data = {
                access:  LocalStorage.get('access'),
                refresh: LocalStorage.get('refresh'),
            };
        break;
        case `${MOCK_ENDPOINT}/user`:
            data = {
                username: LocalStorage.get('mock_username'),
            };
        break;
        default:
            // nothing

    }

    return  Promise.resolve({
        status: 200,
        ok: true,
        text: () => Promise.resolve(JSON.stringify(data)),
        json: () => Promise.resolve(data),
    });
};


const timestamp = function(sec=0) {
    const now  = Math.floor(Date.now()/1000);
    return now + sec;
}

const create_jwt_token = function(claims={}) {
    const header = btoa(JSON.stringify({
        alg: 'HS256',
        typ: 'JWT',
    }));
    const payload = btoa(JSON.stringify(claims));
    const signature = btoa('<NOT IMPLEMENTED>');

    return `${header}.${payload}.${signature}`
};


describe('Auth disabled', () => {

    test('is_protected()', () => {
        const res = Auth.is_protected();
        expect(res).toBe(false);
    });

});

describe('Auth.init()', () => {

    const oldProviderUri = process.env.REACT_APP_JWT_PROVIDER_URI;
    const oldXMLHttpRequest = window.XMLHttpRequest;

    beforeEach(() => {
        process.env.REACT_APP_JWT_PROVIDER_URI = MOCK_ENDPOINT;
        window.fetch = mockFetchApi;
        LocalStorage.clear();
    });

    afterAll(() => {
        process.env.REACT_APP_JWT_PROVIDER_URI = oldProviderUri;
        window.fetch = _cached_fetch;
        LocalStorage.clear();
    });

    test('is_protected()', () => {
        const res = Auth.is_protected();
        expect(res).toBe(true);
    });

    test('init(): no tokens stored', () => {
        return Auth.init().then((res) => {
            expect(res).toBe(null);
        });
    });

    test('init(): access token expired', () => {
        // mock data
        LocalStorage.set('ss_access_token', create_jwt_token({ exp: timestamp(-ONE_HOUR), debug: 'access expired' }));

        return Auth.init().then((res) => {
            expect(res).toBe(null);
        });
    });

    test('init(): access token missing, refresh token expired', () => {
        // mock data
        LocalStorage.set('ss_refresh_token', create_jwt_token({ exp: timestamp(-ONE_HOUR), debug: 'refresh expired' }));

        return Auth.init().then((res) => {
            expect(res).toBe(null);
        });
    });

    test('init(): access token un-expired', () => {
        // mock data
        LocalStorage.set('ss_access_token', create_jwt_token({ exp: timestamp(ONE_HOUR), debug: 'access' }));
        // control value
        const username = `USER_${timestamp()}`;
        LocalStorage.set('mock_username', username);

        return Auth.init().then((res) => {
            expect(Object.prototype.toString.call(res)).toBe('[object Object]');
            expect(res.username).toBe(username);
        });
    });

    test('init(): access token missing, refresh token un-expired', () => {
        // mock data
        LocalStorage.set('ss_refresh_token', create_jwt_token({ exp: timestamp(ONE_HOUR), debug: 'refresh' }));
        // control value
        const username = `USER_${timestamp()}`;
        LocalStorage.set('mock_username', username);

        return Auth.init().then((res) => {
            expect(Object.prototype.toString.call(res)).toBe('[object Object]');
            expect(res.username).toBe(username);
        });
    });

    test('init(none): access token expired, refresh token expired', () => {
        // mock data
        LocalStorage.set('ss_access_token', create_jwt_token({ exp: timestamp(-ONE_HOUR), debug: 'access expired' }));
        LocalStorage.set('ss_refresh_token', create_jwt_token({ exp: timestamp(-ONE_HOUR), debug: 'refresh expired' }));

        return Auth.init().then((res) => {
            expect(res).toBe(null);
        });
    });

    test('init(refresh): access token expired, refresh token un-expired', () => {
        // mock data
        LocalStorage.set('ss_access_token', create_jwt_token({ exp: timestamp(-ONE_HOUR), debug: 'access expired' }));
        LocalStorage.set('ss_refresh_token', create_jwt_token({ exp: timestamp(ONE_HOUR), debug: 'refresh' }));
        // control value
        const username = `USER_${timestamp()}`;
        LocalStorage.set('mock_username', username);

        return Auth.init().then((res) => {
            expect(Object.prototype.toString.call(res)).toBe('[object Object]');
            expect(res.username).toBe(username);
        });
    });

    test('init(access): access token un-expired, refresh token un-expired', () => {
        // mock data
        LocalStorage.set('ss_access_token', create_jwt_token({ exp: timestamp(ONE_HOUR), debug: 'access' }));
        LocalStorage.set('ss_refresh_token', create_jwt_token({ exp: timestamp(ONE_HOUR), debug: 'refresh' }));
        // control value
        const username = `USER_${timestamp()}`;
        LocalStorage.set('mock_username', username);

        return Auth.init().then((res) => {
            expect(Object.prototype.toString.call(res)).toBe('[object Object]');
            expect(res.username).toBe(username);
        });
    });

});
