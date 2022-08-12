import { isScalar, sanitizeKcgiJsonString, isArray, isObject, camelToNormal, buildUrl } from '../Utils';

const days = function(factor) {
    return factor * 86400;
};

const hours = function(factor) {
    return factor * 3600;
};

const minutes = function(factor) {
    return factor * 60;
};

describe('isScalar', () => {

    test('should accept scalar values', () => {
        expect(isScalar(0)).toBe(true);
        expect(isScalar(42)).toBe(true);
        expect(isScalar(Math.PI)).toBe(true);
        expect(isScalar(Infinity)).toBe(true);
        expect(isScalar(NaN)).toBe(true);

        expect(isScalar('')).toBe(true);
        expect(isScalar('a')).toBe(true);
        expect(isScalar('0123')).toBe(true);
        expect(isScalar(42)).toBe(true);
        expect(isScalar('{ "json": true }')).toBe(true);

        expect(isScalar(null)).toBe(true);
        expect(isScalar(false)).toBe(true);
        expect(isScalar(true)).toBe(true);

    });

    test('should reject undefined values and not defined params', () => {
        expect(isScalar(undefined)).toBe(false);
        expect(isScalar()).toBe(false);
    });

    test('should reject complex values', () => {
        expect(isScalar({})).toBe(false);
        expect(isScalar([])).toBe(false);
        expect(isScalar(Symbol())).toBe(false);
        expect(isScalar(Date)).toBe(false);
        expect(isScalar(() => 42)).toBe(false);
    });

});

describe('Misc', () => {
    test('isArray', () => {
        expect(isArray()).toBe(false);
        expect(isArray([])).toBe(true);
        expect(isArray({})).toBe(false);
    });

    test('isObject', () => {
        expect(isObject()).toBe(false);
        expect(isObject([])).toBe(false);
        expect(isObject({})).toBe(true);
        expect(isObject(new Object())).toBe(true);
        expect(isObject(Symbol())).toBe(false);
    });

    test('camelToNormal', () => {
        expect(camelToNormal('testTest')).toBe('test Test');
        expect(camelToNormal('TestTest')).toBe('Test Test');
        expect(camelToNormal(' TestTest ')).toBe('Test Test');
        expect(camelToNormal('TesttesT ')).toBe('Testtes T');
    });
});

describe('Time', () => {
    /* removed
    test('prettyHours', () => {
        expect(prettyHours(days(0) + hours(0)  + minutes(0) )).toBe('12:00:00 am');
        expect(prettyHours(days(0) + hours(11) + minutes(59))).toBe('11:59:00 am');
        expect(prettyHours(days(0) + hours(12) + minutes(0) )).toBe('12:00:00 pm');
        expect(prettyHours(days(0) + hours(12) + minutes(1) )).toBe('12:01:00 pm');
        expect(prettyHours(days(0) + hours(23) + minutes(59))).toBe('11:59:00 pm');
        expect(prettyHours(days(0) + hours(24) + minutes(0) )).toBe('12:00:00 am');

        expect(prettyHours(days(1) + hours(0)  + minutes(1) )).toBe('12:01:00 am +1 day');
        expect(prettyHours(days(1) + hours(11) + minutes(59))).toBe('11:59:00 am +1 day');
        expect(prettyHours(days(1) + hours(12) + minutes(0) )).toBe('12:00:00 pm +1 day');
        expect(prettyHours(days(1) + hours(12) + minutes(1) )).toBe('12:01:00 pm +1 day');
        expect(prettyHours(days(1) + hours(23) + minutes(59))).toBe('11:59:00 pm +1 day');
        expect(prettyHours(days(1) + hours(24) + minutes(0) )).toBe('12:00:00 am +2 days');

        expect(prettyHours(days(2) + hours(0)  + minutes(1) )).toBe('12:01:00 am +2 days');
        expect(prettyHours(days(2) + hours(11) + minutes(59))).toBe('11:59:00 am +2 days');
        expect(prettyHours(days(2) + hours(12) + minutes(0) )).toBe('12:00:00 pm +2 days');
        expect(prettyHours(days(2) + hours(12) + minutes(1) )).toBe('12:01:00 pm +2 days');
        expect(prettyHours(days(2) + hours(23) + minutes(59))).toBe('11:59:00 pm +2 days');
        expect(prettyHours(days(2) + hours(24) + minutes(0) )).toBe('12:00:00 am +3 days');

        expect(prettyHours(days(0) - hours(11) - minutes(59))).toBe('11:59:00 am -1 day');
        expect(prettyHours(days(0) - hours(12) - minutes(0) )).toBe('12:00:00 pm -1 day');
        expect(prettyHours(days(0) - hours(12) - minutes(1) )).toBe('12:01:00 pm -1 day');
        expect(prettyHours(days(0) - hours(23) - minutes(59))).toBe('11:59:00 pm -1 day');
        expect(prettyHours(days(0) - hours(24) - minutes(0) )).toBe('12:00:00 am -1 day');

        expect(prettyHours(days(-1) - hours(0)  - minutes(1) )).toBe('12:01:00 am -2 days');
        expect(prettyHours(days(-1) - hours(11) - minutes(59))).toBe('11:59:00 am -2 days');
        expect(prettyHours(days(-1) - hours(12) - minutes(0) )).toBe('12:00:00 pm -2 days');
        expect(prettyHours(days(-1) - hours(12) - minutes(1) )).toBe('12:01:00 pm -2 days');
        expect(prettyHours(days(-1) - hours(23) - minutes(59))).toBe('11:59:00 pm -2 days');
        expect(prettyHours(days(-1) - hours(24) - minutes(0) )).toBe('12:00:00 am -3 days');
    });
    */
});

describe('sanitizeKcgiJsonString', () => {
    test('sanitize', () => {
        expect(sanitizeKcgiJsonString('description <p><strong>with HTML<\/strong><\/p>')).toBe('description <p><strong>with HTML</strong></p>');
    });
});

describe('buldUrl', () => {
    test('empty', () => {
        expect(buildUrl()).toBe('/');
    });

    test('base_url', () => {
        expect(buildUrl('base')).toBe('base/');
        expect(buildUrl('base/')).toBe('base/'); // strip trailing slash
    });

    test('path', () => {
        expect(buildUrl('base', 'path')).toBe('base/path');
        expect(buildUrl('base', 'path/')).toBe('base/path/');
        expect(buildUrl('base/', '/path')).toBe('base/path');
        expect(buildUrl('base/', '/')).toBe('base/');
    });

    test('params', () => {
        expect(buildUrl('base', 'path', null)).toBe('base/path');
        expect(buildUrl('base', 'path', {})).toBe('base/path');
        expect(buildUrl('base', 'path', { 'param': 'value' })).toBe('base/path?param=value');
        expect(buildUrl('base', 'path/', { 'param': 'value' })).toBe('base/path/?param=value');
    });
});
