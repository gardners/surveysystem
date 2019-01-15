import { makeDisplayGroups, getGroupId } from '../DisplayGroups';

const makeQuestion = function(id) {
    return {
        id,
        name: id,
        type: 'TEXT',
        tile: 'title',
        title_text: 'title-text',
    };
};

describe('getGroupId', () => {

    test('negative (empty gid)', () => {
        let gid;

        gid = getGroupId('');
        expect(gid).toBe('');

        gid = getGroupId('test');
        expect(gid).toBe('test');

        gid = getGroupId('test');
        expect(gid).toBe('test');

        gid = getGroupId('test_');
        expect(gid).toBe('test_');
    });

    test('positive (gid)', () => {
        let gid;


        gid = getGroupId('test__');
        expect(gid).toBe('test');

        gid = getGroupId(' test__');
        expect(gid).toBe('test');

        gid = getGroupId('test__ ');
        expect(gid).toBe('test');

        gid = getGroupId('test__1');
        expect(gid).toBe('test');

        gid = getGroupId('test__1 ');
        expect(gid).toBe('test');

        gid = getGroupId('test__1_2');
        expect(gid).toBe('test');

        gid = getGroupId('test__1__2');
        expect(gid).toBe('test__1');
    });

});

describe('makeDisplayGroups', () => {

    test('make', () => {
        let groups;

        groups = makeDisplayGroups([
            makeQuestion('test1'),
            makeQuestion('test2'),
        ]);
        // console.log(JSON.stringify(qq, null, 2));

        expect(groups[0].length).toBe(1);
        expect(groups[0][0].id).toBe('test1');

        expect(groups[1].length).toBe(1);
        expect(groups[1][0].id).toBe('test2');

        groups = makeDisplayGroups([
            makeQuestion('test_0'),
            makeQuestion('test__1'),
            makeQuestion('test__2'),
            makeQuestion('test_1'),
        ]);

        expect(groups[0].length).toBe(1);
        expect(groups[0][0].id).toBe('test_0');

        //grouped
        expect(groups[1].length).toBe(2);
        expect(groups[1][0].id).toBe('test__1');
        expect(groups[1][1].id).toBe('test__2');

        expect(groups[2].length).toBe(1);
        expect(groups[2][0].id).toBe('test_1');
    });

});
