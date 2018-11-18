const Config = require('../../conf/config');

describe('config', () => {

    test('load', () => {
        expect(Config).toBeTruthy();
    });

    test('object', () => {
        expect(Object.prototype.toString.call(Config)).toBe('[object Object]');
    });

});
