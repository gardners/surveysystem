import { serializeAnswer, serializeAnswerValue, mapTypeToField } from '../../serializer';

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

describe('serializeAnswer: to CSV row', () => {

    let a;

    test('type: text', () => {
        //positive
        expect(serializeAnswer('textid', { text: 'T' })).toBe('textid:T:0:0:0:0:0:0:0');
        expect(serializeAnswer('textid', { text: 1.24 })).toBe('textid:1.24:0:0:0:0:0:0:0');

        expect(serializeAnswer('textid', { text: new String('T') })).toBe('textid:\"T\":0:0:0:0:0:0:0');
        // tests json and escaping quotes and colons
        expect(serializeAnswer('textid', { text: '{ "isJSON": true }' })).toBe('textid:{ \"isJSON\"\: true }:0:0:0:0:0:0:0');
        expect(serializeAnswer('textid', { text: "Tom's" })).toBe("textid:Tom\'s:0:0:0:0:0:0:0");
        expect(serializeAnswer('textid', { text: '  trim text  ' })).toBe('textid:trim text:0:0:0:0:0:0:0');
        // negative
        expect(serializeAnswer('textid')).toBeInstanceOf(Error);
        expect(serializeAnswer('textid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('textid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('textid', { text: '' })).toBeInstanceOf(Error);
        expect(serializeAnswer('textid', { text: null })).toBeInstanceOf(Error);
        expect(serializeAnswer('textid', { text: undefined })).toBeInstanceOf(Error);
        expect(serializeAnswer('textid', { text: Symbol(1) })).toBeInstanceOf(Error);
    });

    test('type: value', () => {
        //positive
        expect(serializeAnswer('numberid', { value: 0 })).toBe('numberid::0:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { value: 1 })).toBe('numberid::1:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { value: 0.1 })).toBe('numberid::0.1:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { value: '123' })).toBe('numberid::123:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { value: new Number(123) })).toBe('numberid::123:0:0:0:0:0:0');
        // negative
        expect(serializeAnswer('numberid')).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('numberid', { value: NaN })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { value: Infinity, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { value: null, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { value: undefined, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { value: Symbol(1), })).toBeInstanceOf(Error);
    });

    test('type: lat', () => {
        // positive
        expect(serializeAnswer('latid', { lat: '0' })).toBe('latid::0:0:0:0:0:0:0');
        expect(serializeAnswer('latid', { lat: '  1.0   ' })).toBe('latid::0:1:0:0:0:0:0');
        expect(serializeAnswer('latid', { lat: 3.14 })).toBe('latid::0:3.14:0:0:0:0:0');
        // negative
        expect(serializeAnswer('latid')).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', undefined)).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('latid', { lat: 'string' })).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', { lat: undefined })).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', { lat: null })).toBeInstanceOf(Error);
        expect(serializeAnswer('latid', { lat: NaN })).toBeInstanceOf(Error);
    });

    test('type: lon', () => {
        // positive
        expect(serializeAnswer('lonid', { lon: '0' })).toBe('lonid::0:0:0:0:0:0:0');
        expect(serializeAnswer('lonid', { lon: '  1.0   ' })).toBe('lonid::0:0:1:0:0:0:0');
        expect(serializeAnswer('lonid', { lon: 3.14 })).toBe('lonid::0:0:3.14:0:0:0:0');
        // negative
        expect(serializeAnswer('lonid')).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', undefined)).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('lonid', { lon: 'string' })).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', { lon: undefined })).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', { lon: null })).toBeInstanceOf(Error);
        expect(serializeAnswer('lonid', { lon: NaN })).toBeInstanceOf(Error);
    });

    xtest('type: datetime', () => {
    });

});

describe('serializeAnswerValue: to CSV row', () => {

    test('type: text', () => {
        //positive
        expect(serializeAnswerValue('textid', 'T', 'text')).toBe('textid:T:0:0:0:0:0:0:0');
        expect(serializeAnswerValue('textid', 1.24, 'text')).toBe('textid:1.24:0:0:0:0:0:0:0');
        expect(serializeAnswerValue('textid', new String('T'), 'text')).toBe('textid:\"T\":0:0:0:0:0:0:0');
        // tests json and escaping quotes and colons
        expect(serializeAnswerValue('textid', '{ "isJSON": true }', 'text')).toBe('textid:{ \"isJSON\"\: true }:0:0:0:0:0:0:0');
        expect(serializeAnswerValue('textid', "Tom's", 'text')).toBe("textid:Tom\'s:0:0:0:0:0:0:0");
        expect(serializeAnswerValue('textid', '  trim text  ', 'text')).toBe('textid:trim text:0:0:0:0:0:0:0');
        // negative
        expect(serializeAnswerValue('textid', '', 'text')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('textid', null, 'text')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('textid', undefined, 'text')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('textid', Symbol(1), 'text')).toBeInstanceOf(Error);
    });

    test('type: value', () => {
        //positive
        expect(serializeAnswerValue('numberid', 0, 'value')).toBe('numberid::0:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', 1, 'value')).toBe('numberid::1:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', 0.1, 'value')).toBe('numberid::0.1:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', '123', 'value')).toBe('numberid::123:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', new Number(123), 'value')).toBe('numberid::123:0:0:0:0:0:0');
        // negative
        expect(serializeAnswerValue('numberid', NaN, 'value')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', Infinity, 'value')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', null, 'value')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', undefined, 'value')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', Symbol(1), 'value')).toBeInstanceOf(Error);
    });

    test('type: lat', () => {
        // positive
        expect(serializeAnswerValue('latid', '0', 'lat')).toBe('latid::0:0:0:0:0:0:0');
        expect(serializeAnswerValue('latid', '1.0,', 'lat')).toBe('latid::0:1:0:0:0:0:0');
        expect(serializeAnswerValue('latid', '  1.0  ', 'lat')).toBe('latid::0:1:0:0:0:0:0');
        expect(serializeAnswerValue('latid', 3.14, 'lat')).toBe('latid::0:3.14:0:0:0:0:0');
        // negative
        expect(serializeAnswerValue('latid', 'string', 'lat')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('latid', undefined, 'lat')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('latid', null, 'lat')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('latid', NaN, 'lat')).toBeInstanceOf(Error);
    });

    test('type: lon', () => {
        // positive
        expect(serializeAnswerValue('lonid', '0', 'lon')).toBe('lonid::0:0:0:0:0:0:0');
        expect(serializeAnswerValue('lonid', '1.0,', 'lon')).toBe('lonid::0:0:1:0:0:0:0');
        expect(serializeAnswerValue('lonid', '  1.0  ', 'lon')).toBe('lonid::0:0:1:0:0:0:0');
        expect(serializeAnswerValue('lonid', 3.14, 'lon')).toBe('lonid::0:0:3.14:0:0:0:0');
        // negative
        expect(serializeAnswerValue('lonid', 'string', 'lon')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('lonid', undefined, 'lon')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('lonid', null, 'lon')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('lonid', NaN, 'lon')).toBeInstanceOf(Error);
    });

    xtest('type: datetime', () => {
    });

});

describe('mapTypeToField', () => {
    let fn;
    test('undefined value', () => {
        fn = mapTypeToField('TEXT');
        expect(fn()).toMatchObject({
            text: undefined,
        });
    });

    // types

    test('TEXT', () => {
        fn = mapTypeToField('TEXT');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('TEXT', () => {
        fn = mapTypeToField('TEXT');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('HIDDEN', () => {
        fn = mapTypeToField('HIDDEN');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('TEXTAREA', () => {
        fn = mapTypeToField('TEXTAREA');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('MULTICHOICE <string>', () => {
        fn = mapTypeToField('MULTICHOICE');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('MULTICHOICE <array>', () => {
        fn = mapTypeToField('MULTICHOICE');
        expect(fn(['test1', 'test2'])).toMatchObject({
            text: 'test1,test2',
        });
    });

    test('MULTISELECT <string>', () => {
        fn = mapTypeToField('MULTISELECT');
        expect(fn('test')).toMatchObject({
            text: 'test',
        });
    });

    test('MULTISELECT <array>', () => {
        fn = mapTypeToField('MULTISELECT');
        expect(fn(['test1', 'test2'])).toMatchObject({
            text: 'test1,test2',
        });
    });

    test('INT', () => {
        fn = mapTypeToField('INT');
        expect(fn(5)).toMatchObject({
            value: 5,
        });
    });

    test('FIXEDPOINT', () => {
        fn = mapTypeToField('FIXEDPOINT');
        expect(fn(1.0)).toMatchObject({
            value: 1.0,
        });
    });

    test('LATLON', () => {
        fn = mapTypeToField('LATLON');
        expect(fn(1.0, 1.1)).toMatchObject({
            lat: 1.0,
            lon: 1.1,
        });
    });

    // TODO 'DATETIME' fcgimain.c puts it into "text" clarify
    test('DATETIME', () => {
        fn = mapTypeToField('DATETIME');
        expect(fn(6800, 3600)).toMatchObject({
            time_begin: 6800,
            time_zone_delta: 3600,
        });
    });

    // TODO fcgimain.cclarify
    test('TIMERANGE', () => {
        fn = mapTypeToField('TIMERANGE');
        expect(fn(0, 6800)).toMatchObject({
            time_begin: 0,
            time_end: 6800,
            time_zone_delta: 0,
        });

        fn = mapTypeToField('TIMERANGE');
        expect(fn(0, 6800, 3600)).toMatchObject({
            time_begin: 0,
            time_end: 6800,
            time_zone_delta: 3600,
        });
    });

    test('EMAIL', () => {
        fn = mapTypeToField('EMAIL');
        expect(fn('email')).toMatchObject({
            text: 'email',
        });
    });

    test('CHECKBOX', () => {
        fn = mapTypeToField('CHECKBOX');
        expect(fn('checked')).toMatchObject({
            text: 'checked',
        });
    });

    xtest('UPLOAD', () => {
        // TODO
    });
});
