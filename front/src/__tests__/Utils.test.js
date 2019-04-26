import { isScalar, sanitizeKcgiJsonString, isArray, isObject, camelToNormal } from '../Utils';

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

describe('sanitizeKcgiJsonString', () => {
    test('sanitize', () => {
        expect(sanitizeKcgiJsonString('description <p><strong>with HTML<\/strong><\/p>')).toBe('description <p><strong>with HTML</strong></p>');
    });
});
