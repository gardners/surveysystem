import { serializeAnswer } from '../payload-serializer';

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

describe('serialize to CSV row', () => {

    test('type: text', () => {
        //positive
        expect(serializeAnswer('text', 'T', 'text')).toBe('text:T:0:0:0:0:0:0:0');
        expect(serializeAnswer('text', 1.24, 'text')).toBe('text:1.24:0:0:0:0:0:0:0');
        expect(serializeAnswer('text', new String('T'), 'text')).toBe('text:\"T\":0:0:0:0:0:0:0');
        // tests json and escaping quotes and colons
        expect(serializeAnswer('text', '{ "isJSON": true }', 'text')).toBe('text:{ \"isJSON\"\: true }:0:0:0:0:0:0:0');
        expect(serializeAnswer('text', "Tom's", 'text')).toBe("text:Tom\'s:0:0:0:0:0:0:0");
        expect(serializeAnswer('text', '  trim text  ', 'text')).toBe('text:trim text:0:0:0:0:0:0:0');
        // negative
        expect(serializeAnswer('text', '', 'text')).toBeInstanceOf(Error);
        expect(serializeAnswer('text', null, 'text')).toBeInstanceOf(Error);
        expect(serializeAnswer('text', undefined, 'text')).toBeInstanceOf(Error);
        expect(serializeAnswer('text', Symbol(1), 'text')).toBeInstanceOf(Error);
    });

    test('type: number', () => {
        //positive
        expect(serializeAnswer('number', 0, 'number')).toBe('number::0:0:0:0:0:0:0');
        expect(serializeAnswer('number', 1, 'number')).toBe('number::1:0:0:0:0:0:0');
        expect(serializeAnswer('number', 0.1, 'number')).toBe('number::0.1:0:0:0:0:0:0');
        expect(serializeAnswer('number', '123', 'number')).toBe('number::123:0:0:0:0:0:0');
        expect(serializeAnswer('number', new Number(123), 'number')).toBe('number::123:0:0:0:0:0:0');
        // negative
        expect(serializeAnswer('number', NaN, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', Infinity, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', null, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', undefined, 'number')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', Symbol(1), 'number')).toBeInstanceOf(Error);
    });

    test('type: geolocation', () => {
        // positive
        expect(serializeAnswer('id', '0,0', 'geolocation')).toBe('id::0:0:0:0:0:0:0');
        expect(serializeAnswer('id', '1.0,0.1', 'geolocation')).toBe('id::0:1:0.1:0:0:0:0');
        expect(serializeAnswer('id', '  1.0,  0.1  ', 'geolocation')).toBe('id::0:1:0.1:0:0:0:0');
        // negative
        expect(serializeAnswer('number', '0.2,string', 'geolocation')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', 'string,0.2', 'geolocation')).toBeInstanceOf(Error);
        expect(serializeAnswer('number', NaN, 'geolocation')).toBeInstanceOf(Error);
    });

    xtest('type: datetime', () => {
    });

});
