import { FieldTypes, mapQuestionType } from '../payload-mapper';

describe('FieldTypes', () => {
    test('FieldTypes is frozen', () => {
        expect(() => {
            FieldTypes.newProperty = 'test'
        }).toThrow();
    });
});

describe('mapQuestionType', () => {
    test('returns "text" if poperty cannot be found', () => {
        const dontExists = '' + Date.now();
        expect(mapQuestionType(dontExists)).toBe('text');
        const exists = 'text';
        expect(mapQuestionType(exists)).toBe('text');
    });
});
