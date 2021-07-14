import Storage from '../../storage/LocalStorage';

const KEY = '__test__';

describe('LocalStorage', () => {

    afterAll(() => {
        window.localStorage.removeItem(KEY);
    });

    test('store complex data', () => {
        const data = [1, 2, 3];

        Storage.set(KEY, data);
        const res = localStorage.getItem(KEY);

        expect(res).toBe(JSON.stringify(data));
    });

    test('store stringified JSON', () => {
        const data = JSON.stringify([1, 2, 3]);

        Storage.set(KEY, data);
        const res = localStorage.getItem(KEY);
        expect(res).toBe(JSON.stringify(data));
    });

    test('fetch invalid JSON', () => {
        const json = '{ invalid }';

        localStorage.setItem(KEY, json);
        const res = Storage.get(KEY);
        expect(res).toStrictEqual(null);
    });

    test('store and fetch undefined data', () => {
        const data = undefined;

        Storage.set(KEY, data);
        const res = Storage.get(KEY);
        expect(res).toStrictEqual(null);
    });

});
