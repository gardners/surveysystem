import Storage from '../../storage/LocalStorage';
import 'jest-localstorage-mock';

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
        const json = JSON.stringify([1, 2, 3]);

        Storage.set(KEY, json);
        const res = localStorage.getItem(KEY);

        it('shouldn\'t double encode JSON strings', () => {
            expect(res).toBe(JSON.stringify(data));
        })

    });

    test('fetch invalid JSON', () => {
        const json = '{ invalid }';

        localStorage.setItem(KEY, json);
        const res = Storage.get(KEY);

        it('should return NULL in case the stored value is invalid json', () => {
            expect(res).toStrictEqual(null);
        })

    });

    test('store and fetch undefined data', () => {
        const data = undefined;

        Storage.set(KEY, data);
        const res = Storage.get(KEY);

        it('should return NULL in case the stored value is invalid json', () => {
            expect(res).toStrictEqual(null);
        })

    });

});
