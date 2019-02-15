import Log from '../Log';

let INVOKED = '';

const fn1 = function(...args) {
    INVOKED = (INVOKED) ? INVOKED += ' fn1' : 'fn1';
    return args;
};

const fn2 = function(...args) {
    INVOKED = (INVOKED) ? INVOKED += ' fn2' : 'fn2';
    return args;
};

describe('Log', () => {
    test('is an instance', () => {
        expect(Log.constructor.name).toBe('Log');
        expect(typeof(Log.log)).toBe('function');
    });

    test('logging', () => {
        let m;

        m = Log.add('message', 'TEST');
        expect(JSON.stringify(m)).toBe('{"severity":"TEST","message":"message"}');

        m = Log.log('message');
        expect(JSON.stringify(m)).toBe('{"severity":"info","message":"message"}');

        m = Log.error('message');
        expect(JSON.stringify(m)).toBe('{"severity":"error","message":"message"}');

        m = Log.warn('message');
        expect(JSON.stringify(m)).toBe('{"severity":"warn","message":"message"}');

        m = Log.debug('message');
        expect(JSON.stringify(m)).toBe('{"severity":"debug","message":"message"}');
    });

    test('logging errors always sets everity "error"', () => {
        let m;
        const err = new Error('message');

        m = Log.add(err, 'TEST');
        expect(JSON.stringify(m)).toBe('{"severity":"error","message":"Error: message"}');
    });

    test('subscribe fn1', () => {
        // subscribe first
        Log.subscribe('fn1', fn1);
        expect(Object.keys(Log.subscribers).toString()).toBe('fn1');
        expect(Log.subscribers.fn1).toBe(fn1);
    });

    test('subscribe fn2', () => {
        // subscribe second
        Log.subscribe('fn2', fn2);
        expect(Object.keys(Log.subscribers).toString()).toBe('fn1,fn2');
        expect(Log.subscribers.fn1).toBe(fn1);
        expect(Log.subscribers.fn2).toBe(fn2);
    });

    test('subscribtion calls and order', () => {
        Log.log('invoke fn2');
        expect(INVOKED).toBe('fn1 fn2');
    });

    test('unsubscribe fn1', () => {
        INVOKED = ''; // reset

        Log.unsubscribe('fn1');
        expect(Object.keys(Log.subscribers).toString()).toBe('fn2');

        Log.log('invoke fn2 again');
        expect(INVOKED).toBe('fn2');
    });

    test('unsubscribe fn2', () => {
        INVOKED = ''; // reset

        Log.unsubscribe('fn2');
        expect(Object.keys(Log.subscribers).toString()).toBe('');

        Log.log('invoke fn1 again');
        expect(INVOKED).toBe('');
    });
});
