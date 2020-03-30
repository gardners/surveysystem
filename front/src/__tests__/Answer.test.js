import Answer from '../Answer';

/*
 * @see backend/src/question_types.c
 *
 * DESERIALISE_STRING(a->uid);
 * DESERIALISE_STRING(a->text);
 * DESERIALISE_LONGLONG(a->value);
 * DESERIALISE_LONGLONG(a->lat);
 * DESERIALISE_LONGLONG(a->lon);
 * DESERIALISE_LONGLONG(a->time_begin);
 * DESERIALISE_LONGLONG(a->time_end);
 * DESERIALISE_INT(a->time_zone_delta);
 * DESERIALISE_INT(a->dst_delta);
 */

const make_answer = function(merge) {
    return Object.assign(Answer.model(), merge);
};

const run_qestiontype_answer__text = function(type) {

    test(`question type: ${type}`, () => {
        const uid = 'textid';
        const q = { id: uid, type };
        const unit = '';

        //positive
        expect(Answer.setValue(q, 'T')).toMatchObject({ uid, text: 'T', unit });
        expect(Answer.setValue(q, 1.24)).toMatchObject({ uid, text: 1.24, unit });
        expect(Answer.setValue(q, '')).toMatchObject({ uid, text: '', unit });
        expect(Answer.setValue(q, String('T'))).toMatchObject({ uid, text: 'T', unit });
        expect(Answer.setValue(q, ' T ')).toMatchObject({ uid, text: ' T ', unit });

        // negative
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, Symbol(1))).toBeInstanceOf(Error);
        expect(Answer.setValue(q, new String('T'))).toBeInstanceOf(Error);
    });

};

const run_qestiontype_answer__text__commaseparated = function(type) {

    test(`question type: ${type}`, () => {
        const uid = 'textid';
        const q = { id: uid, type };
        const unit = '';

        //positive
        expect(Answer.setValue(q, [])).toMatchObject({ uid, text: '', unit });
        expect(Answer.setValue(q, ['T'])).toMatchObject({ uid, text: 'T', unit });
        expect(Answer.setValue(q, ['A', 'B'])).toMatchObject({ uid, text: 'A,B', unit });
        expect(Answer.setValue(q, [1.24])).toMatchObject({ uid, text: '1.24', unit });
        expect(Answer.setValue(q, [1, 2])).toMatchObject({ uid, text: '1,2', unit });
        expect(Answer.setValue(q, [''])).toMatchObject({ uid, text: '', unit });
        expect(Answer.setValue(q, ['', ''])).toMatchObject({ uid, text: ',', unit });
        expect(Answer.setValue(q, [String('A'), String('B')])).toMatchObject({ uid, text: 'A,B', unit });
        expect(Answer.setValue(q, [' A ', ' B '])).toMatchObject({ uid, text: ' A , B ', unit });

        expect(Answer.setValue(q, 'string')).toMatchObject({ uid, text: 'string', unit });

        // negative
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 2)) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, Symbol(1))).toBeInstanceOf(Error);
        expect(Answer.setValue(q, new String('T'))).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [null, 'A'])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', null])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [undefined, 'A'])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', undefined])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [{}, 'A'])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', {}])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [[], 'A'])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', []])) .toBeInstanceOf(Error);

        expect(Answer.setValue(q, [Symbol(1), 'A'])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', Symbol(1)])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [new String('T'), 'A'])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, ['A', new String('T')])).toBeInstanceOf(Error);
    });

};

const run_qestiontype_answer__value = function(type, unit) {

    test(`question type: ${type}`, () => {
        const uid = 'numberid';
        const q = { id: uid, type };

        //positive
        expect(Answer.setValue(q, 0)).toMatchObject({ uid, value: 0, unit });
        expect(Answer.setValue(q, 0.1)).toMatchObject({ uid, value: 0.1, unit });
        expect(Answer.setValue(q, -0.1)).toMatchObject({ uid, value: -0.1, unit });
        expect(Answer.setValue(q, Number('123'))).toMatchObject({ uid, value: 123, unit });
        expect(Answer.setValue(q, '123')).toMatchObject({ uid, value: 123, unit });
        expect(Answer.setValue(q, '')).toMatchObject({ uid, value: 0, unit });

        // negative
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 'string')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, Symbol(1))).toBeInstanceOf(Error);
        expect(Answer.setValue(q, NaN)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, Infinity)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, new Number('123'))).toBeInstanceOf(Error);
    });

};

const run_qestiontype_answer__time_begin = function(type) {
    const unit = 'seconds';

    test(`question type: ${type}`, () => {
        const uid = 'timestampid';
        const q = { id: uid, type };

        //positive
        expect(Answer.setValue(q, 0)).toMatchObject({ uid, time_begin: 0, unit });
        expect(Answer.setValue(q, 5)).toMatchObject({ uid, time_begin: 5, unit });
        expect(Answer.setValue(q, Number('123'))).toMatchObject({ uid, time_begin: 123, unit });

        // negative
        expect(Answer.setValue(q, '123')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 0.1)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, -1)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, '123')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, '')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 'string')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, Symbol(1))).toBeInstanceOf(Error);
        expect(Answer.setValue(q, NaN)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, Infinity)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, new Number('123'))).toBeInstanceOf(Error);
    });

    test(`force unit: ${unit}`, () => {
        const uid = 'timestampunitid';
        expect(Answer.setValue({ id: uid, type }, 0)).toMatchObject({ uid, time_end: 0, unit });
        expect(Answer.setValue({ id: uid, type, unit: 'overwrite' }, 0)).toMatchObject({ uid, time_end: 0, unit });
    });

};

const run_qestiontype_answer__time_end = function(type) {
    const unit = 'seconds';

    test(`question type: ${type}`, () => {
        const uid = 'timestampid';
        const q = { id: uid, type };


        //positive
        expect(Answer.setValue(q, 0)).toMatchObject({ uid, time_end: 0, unit });
        expect(Answer.setValue(q, 5)).toMatchObject({ uid, time_end: 5, unit });
        expect(Answer.setValue(q, Number('123'))).toMatchObject({ uid, time_end: 123, unit });

        // negative
        expect(Answer.setValue(q, '123')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 0.1)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, -1)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, '123')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, '')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 'string')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, Symbol(1))).toBeInstanceOf(Error);
        expect(Answer.setValue(q, NaN)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, Infinity)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, new Number('123'))).toBeInstanceOf(Error);
    });

    test(`force unit: ${unit}`, () => {
        const uid = 'timestampunitid';
        expect(Answer.setValue({ id: uid, type }, 0)).toMatchObject({ uid, time_end: 0, unit });
        expect(Answer.setValue({ id: uid, type, unit: 'overwrite' }, 0)).toMatchObject({ uid, time_end: 0, unit });
    });

};

const run_qestiontype__TIMERANGE = function() {
    const type = 'TIMERANGE';
    const unit = 'seconds';

    test(`question type: ${type}`, () => {
        const uid = 'timestampid';
        const q = { id: uid, type };

        //positive
        expect(Answer.setValue(q, [0, 0])).toMatchObject({ uid, time_begin: 0, time_end: 0, unit });
        expect(Answer.setValue(q, [5, 0])).toMatchObject({ uid, time_begin: 5, time_end: 0, unit });
        expect(Answer.setValue(q, [ Number('123'), 125])).toMatchObject({ uid, time_begin: 123, time_end: 125, unit });

        // negative
        expect(Answer.setValue(q, [])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 'invalid')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 2)).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [0.1, 5])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [5, 0.1])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [-1, 2])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [2, -1])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [null, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, null])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['123', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, '123'])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [undefined, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, undefined])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, ''])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['string', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, 'string'])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [{}, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, {}])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [[], 1])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, []])) .toBeInstanceOf(Error);

        expect(Answer.setValue(q, [Symbol(1), 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, Symbol(1)])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [NaN, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, NaN])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [Infinity, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, Infinity])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [new Number('123'), 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, new Number('123')])).toBeInstanceOf(Error);
    });

    test(`force unit: ${unit}`, () => {
        const uid = 'timestampunitid';
        expect(Answer.setValue({ id: uid, type }, [0, 0])).toMatchObject({ uid, time_begin: 0, time_end: 0, unit });
        expect(Answer.setValue({ id: uid, type, unit: 'overwrite' }, [0, 0])).toMatchObject({ uid, time_begin: 0, time_end: 0, unit });
    });

};

const run_qestiontype__LATLON = function() {
    const type = 'LATLON';
    const unit = 'degrees';

    test(`question type: ${type}`, () => {
        const uid = 'latlonid';
        const q = { id: uid, type };

        //positive
        expect(Answer.setValue(q, [0, 0])).toMatchObject({ uid, lat: 0, lon: 0, unit });
        expect(Answer.setValue(q, [5, 0])).toMatchObject({ uid, lat: 5, lon: 0, unit });
        expect(Answer.setValue(q, [-90, -180])).toMatchObject({ uid, lat: -90, lon: -180, unit });
        expect(Answer.setValue(q, [90, 180])).toMatchObject({ uid, lat: 90, lon: 180, unit });
        expect(Answer.setValue(q, [ Number('89'), 125])).toMatchObject({ uid, lat: 89, lon: 125, unit });

        // negative
        expect(Answer.setValue(q, [])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, {})).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 'invalid')).toBeInstanceOf(Error);
        expect(Answer.setValue(q, null)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, undefined)).toBeInstanceOf(Error);
        expect(Answer.setValue(q, 2)).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [90.1, 45])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [45, 180.1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [-90.1, 45])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [45, -180.1])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [null, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, null])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['123', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, '123'])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [undefined, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, undefined])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, ''])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, ['string', 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, 'string'])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [{}, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, {}])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [[], 1])) .toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, []])) .toBeInstanceOf(Error);

        expect(Answer.setValue(q, [Symbol(1), 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, Symbol(1)])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [NaN, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, NaN])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [Infinity, 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, Infinity])).toBeInstanceOf(Error);

        expect(Answer.setValue(q, [new Number('123'), 1])).toBeInstanceOf(Error);
        expect(Answer.setValue(q, [1, new Number('123')])).toBeInstanceOf(Error);
    });

    test(`force unit: ${unit}`, () => {
        const uid = 'latlonunitid';
        expect(Answer.setValue({ id: uid, type }, [0, 0])).toMatchObject({ uid, lat: 0, lon: 0, unit });
        expect(Answer.setValue({ id: uid, type, unit: 'overwrite' }, [0, 0])).toMatchObject({ uid, lat: 0, lon: 0, unit });
    });

};

describe('Answer.setValue()', () => {

    test('question object requirements', () => {
        expect(Answer.serialize(undefined, 'donotset')) .toBeInstanceOf(Error);
        expect(Answer.serialize('invalid', 'donotset')) .toBeInstanceOf(Error);
        expect(Answer.serialize(null, 'donotset'))      .toBeInstanceOf(Error);
        expect(Answer.serialize({}, 'donotset'))        .toBeInstanceOf(Error);

        expect(Answer.serialize({ type: 'TEXT' }, 'donotset'))                   .toBeInstanceOf(Error);
        expect(Answer.serialize({ type: 'TEXT', id: null }, 'donotset'))         .toBeInstanceOf(Error);
        expect(Answer.serialize({ type: 'TEXT', id: ['invalid'] }, 'donotset'))  .toBeInstanceOf(Error);

        expect(Answer.serialize({ id: 'textid' }, 'donotset'))                   .toBeInstanceOf(Error);
        expect(Answer.serialize({ id: 'textid', type: null }, 'donotset'))       .toBeInstanceOf(Error);
        expect(Answer.serialize({ id: 'textid', type: 'DOES_NOT_EXISTS' }, 'donotset'))  .toBeInstanceOf(Error);
    });

    run_qestiontype_answer__text('TEXT');
    run_qestiontype_answer__text('HIDDEN');
    run_qestiontype_answer__text('TEXTAREA');
    run_qestiontype_answer__text('EMAIL');
    run_qestiontype_answer__text('PASSWORD');
    run_qestiontype_answer__text('CHECKBOX');
    run_qestiontype_answer__text('SINGLECHOICE');
    run_qestiontype_answer__text('SINGLESELECT');

    run_qestiontype_answer__text__commaseparated('MULTICHOICE');
    run_qestiontype_answer__text__commaseparated('MULTISELECT');

    run_qestiontype_answer__value('INT', '');
    run_qestiontype_answer__value('FIXEDPOINT', '');

    run_qestiontype_answer__time_begin('DATETIME');
    run_qestiontype_answer__time_begin('DAYTIME');

    run_qestiontype_answer__time_begin('DATETIME');
    run_qestiontype_answer__time_begin('DAYTIME');
});

describe('Answer.serialize()', () => {

    test('uid required', () => {
        let a;
        //positive
        a = make_answer({ uid: 'test' });
        expect(Answer.serialize(a)).toBe('test::0:0:0:0:0:0:0:');

        // negative
        a = make_answer({ uid: '' });
        expect(Answer.serialize(a)).toBeInstanceOf(Error);
        a = make_answer({ uid: null });
        expect(Answer.serialize(a)).toBeInstanceOf(Error);
        a = make_answer({ uid: 2 });
        expect(Answer.serialize(a)).toBeInstanceOf(Error);
        a = make_answer({ uid: {} });
        expect(Answer.serialize(a)).toBeInstanceOf(Error);

    });

    test('answer completeness', () => {
        let a;

        // negative
        a = { uid: 'test' };
        expect(Answer.serialize(a)).toBeInstanceOf(Error);

        a = make_answer({ uid: 'test' });
        delete a.unit;
        expect(Answer.serialize(a)).toBeInstanceOf(Error);

        a = make_answer({ uid: 'test' });
        a.additionalProperty = true;
        expect(Answer.serialize(a)).toBeInstanceOf(Error);
    });

    test('answer completeness', () => {
        let a;

        // negative
        a = { uid: 'test' };
        expect(Answer.serialize(a)).toBeInstanceOf(Error);

        a = make_answer({ uid: 'test' });
        delete a.unit;
        expect(Answer.serialize(a)).toBeInstanceOf(Error);

        a = make_answer({ uid: 'test' });
        a.additionalProperty = true;
        expect(Answer.serialize(a)).toBeInstanceOf(Error);
    });

    // We do not test validation of values: see tests Answer.setvalue().
    test('question types', () => {
        let a;

        a = Answer.setValue({ id: 'int', type: 'INT', unit: 'u' }, 2);
        expect(Answer.serialize(a)).toBe('int::2:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'fixedpoint', type: 'FIXEDPOINT', unit: 'u' }, .2);
        expect(Answer.serialize(a)).toBe('fixedpoint::0.2:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'multichoice', type: 'MULTICHOICE', unit: 'u' }, ['A', 'B']);
        expect(Answer.serialize(a)).toBe('multichoice:A,B:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'multiselect', type: 'MULTISELECT', unit: 'u' }, ['A', 'B']);
        expect(Answer.serialize(a)).toBe('multiselect:A,B:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'latlon', type: 'LATLON', unit: 'overwritten' }, [2, 3]);
        expect(Answer.serialize(a)).toBe('latlon::0:2:3:0:0:0:0:degrees');

        a = Answer.setValue({ id: 'datetime', type: 'DATETIME', unit: 'overwritten' }, 2);
        expect(Answer.serialize(a)).toBe('datetime::0:0:0:2:0:0:0:seconds');

        a = Answer.setValue({ id: 'timerange', type: 'TIMERANGE', unit: 'overwritten' }, [1, 2]);
        expect(Answer.serialize(a)).toBe('timerange::0:0:0:1:2:0:0:seconds');

        a = Answer.setValue({ id: 'text', type: 'TEXT', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('text:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'checkbox', type: 'CHECKBOX', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('checkbox:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'hidden', type: 'HIDDEN', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('hidden:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'textarea', type: 'TEXTAREA', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('textarea:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'email', type: 'EMAIL', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('email:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'password', type: 'PASSWORD', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('password:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'singlechoice', type: 'SINGLECHOICE', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('singlechoice:T:0:0:0:0:0:0:0:u');

        a = Answer.setValue({ id: 'singleselect', type: 'SINGLESELECT', unit: 'u' }, 'T');
        expect(Answer.serialize(a)).toBe('singleselect:T:0:0:0:0:0:0:0:u');
    });

    test('sanitize line breaks', () => {
        let a;
        //positive
        a = make_answer({ uid: 'test',
            text: 'first line\nnew line'
        });
        expect(Answer.serialize(a)).toBe('test:first line new line:0:0:0:0:0:0:0:');
    });

    test('sanitize apostrophes', () => {
        let a;
        //positive
        a = make_answer({ uid: 'test',
            text: `don't know`
        });
        expect(Answer.serialize(a)).toBe('test:don\'t know:0:0:0:0:0:0:0:');
    });

    test('sanitize backslashes', () => {
        let a;
        //positive
        a = make_answer({ uid: 'test',
            text: `back\slash`
        });
        console.log(Answer.serialize(a));
        expect(Answer.serialize(a)).toBe('test:backslash:0:0:0:0:0:0:0:');
    });

});


describe('Answer.deserialize()', () => {

    test('csv fragment completeness', () => {
        // negative
        expect(Answer.deserialize({})).toBeInstanceOf(Error);
        expect(Answer.deserialize(9)).toBeInstanceOf(Error);
        expect(Answer.deserialize('')).toBeInstanceOf(Error);
        expect(Answer.deserialize('incomplete')).toBeInstanceOf(Error);
        expect(Answer.deserialize('incomplete:answer')).toBeInstanceOf(Error);
        // positive
        expect(Answer.deserialize('test::0:0:0:0:0:0:0:')).toMatchObject({ uid: 'test' });
    });

    test('csv fragment escaped colon', () => {
        expect(Answer.deserialize('test:my answer is\\: test:0:0:0:0:0:0:0:')).toMatchObject({
            uid : 'test',
            text : 'my answer is: test',
            value : 0.0,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });
    });

    test('answer.uid (required)', () => {
        //positive
        expect(Answer.deserialize('test::0:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : '',
            value : 0.0,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });
        //negative
        expect(Answer.deserialize(':::::::::')).toBeInstanceOf(Error);
    });

    test('answer.text', () => {
        expect(Answer.deserialize('test:text:0:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : 'text',
            value : 0.0,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });

        expect(Answer.deserialize('test:123:0:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : '123',
            value : 0.0,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });
    });

    test('answer.value', () => {
        // 0
        expect(Answer.deserialize('test::0:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : '',
            value : 0.0,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });

        expect(Answer.deserialize('test::123:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : '',
            value : 123,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });

        expect(Answer.deserialize('test::3.14:0:0:0:0:0:0:')).toEqual({
            uid : 'test',
            text : '',
            value : 3.14,
            lat : 0.0,
            lon : 0.0,
            time_begin : 0,
            time_end : 0,
            time_zone_delta : 0,
            dst_delta : 0,
            unit: '',
        });

        expect(Answer.deserialize('test::pi=3.14:0:0:0:0:0:0:')).toBeInstanceOf(Error);
    });

    // TODO, all props

});


