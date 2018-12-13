import { serializeAnswer, serializeAnswerValue } from '../payload-serializer';

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

    test('type: number', () => {
        //positive
        expect(serializeAnswer('numberid', { number: 0 })).toBe('numberid::0:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { number: 1 })).toBe('numberid::1:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { number: 0.1 })).toBe('numberid::0.1:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { number: '123' })).toBe('numberid::123:0:0:0:0:0:0');
        expect(serializeAnswer('numberid', { number: new Number(123) })).toBe('numberid::123:0:0:0:0:0:0');
        // negative
        expect(serializeAnswer('numberid')).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('numberid', { number: NaN })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { number: Infinity, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { number: null, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { number: undefined, })).toBeInstanceOf(Error);
        expect(serializeAnswer('numberid', { number: Symbol(1), })).toBeInstanceOf(Error);
    });

    test('type: geolocation', () => {
        // positive
        expect(serializeAnswer('geolocationid', { geolocation: '0,0' })).toBe('geolocationid::0:0:0:0:0:0:0');
        expect(serializeAnswer('geolocationid', { geolocation: '1.0,0.1' })).toBe('geolocationid::0:1:0.1:0:0:0:0');
        expect(serializeAnswer('geolocationid', { geolocation: '  1.0,  0.1  ' })).toBe('geolocationid::0:1:0.1:0:0:0:0');
        // negative
        expect(serializeAnswer('geolocationid')).toBeInstanceOf(Error);
        expect(serializeAnswer('geolocationid', null)).toBeInstanceOf(Error);
        expect(serializeAnswer('geolocationid', 'invalid')).toBeInstanceOf(Error);

        expect(serializeAnswer('geolocationid', { geolocation: '0.2,string' })).toBeInstanceOf(Error);
        expect(serializeAnswer('geolocationid', { geolocation: 'string,0.2' })).toBeInstanceOf(Error);
        expect(serializeAnswer('geolocationid', { geolocation: NaN })).toBeInstanceOf(Error);
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

    test('type: number', () => {
        //positive
        expect(serializeAnswerValue('numberid', 0, 'number')).toBe('numberid::0:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', 1, 'number')).toBe('numberid::1:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', 0.1, 'number')).toBe('numberid::0.1:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', '123', 'number')).toBe('numberid::123:0:0:0:0:0:0');
        expect(serializeAnswerValue('numberid', new Number(123), 'number')).toBe('numberid::123:0:0:0:0:0:0');
        // negative
        expect(serializeAnswerValue('numberid', NaN, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', Infinity, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', null, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', undefined, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', Symbol(1), 'number')).toBeInstanceOf(Error);
    });

    test('type: geolocation', () => {
        // positive
        expect(serializeAnswerValue('geolocationid', '0,0', 'geolocation')).toBe('geolocationid::0:0:0:0:0:0:0');
        expect(serializeAnswerValue('geolocationid', '1.0,0.1', 'geolocation')).toBe('geolocationid::0:1:0.1:0:0:0:0');
        expect(serializeAnswerValue('geolocationid', '  1.0,  0.1  ', 'geolocation')).toBe('geolocationid::0:1:0.1:0:0:0:0');
        // negative
        expect(serializeAnswerValue('numberid', '0.2,string', 'geolocation')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', 'string,0.2', 'geolocation')).toBeInstanceOf(Error);
        expect(serializeAnswerValue('numberid', NaN, 'geolocation')).toBeInstanceOf(Error);
    });

    xtest('type: datetime', () => {
    });

});
