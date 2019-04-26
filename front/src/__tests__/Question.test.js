import { mapQuestionGroups, getGroupId, findQuestionGroupCommons } from '../Question';

const makeQuestion = function(id, type = 'TEXT') {
    return {
        id,
        name: id,
        type,
        tile: 'title',
        description: 'title-text',
    };
};

describe('getGroupId', () => {

    test('negative (empty gid)', () => {
        let gid;

        gid = getGroupId('');
        expect(gid).toBe('');

        gid = getGroupId('test');
        expect(gid).toBe('');

        gid = getGroupId('test');
        expect(gid).toBe('');

        gid = getGroupId('test_');
        expect(gid).toBe('');

        gid = getGroupId('test__');
        expect(gid).toBe('');

        gid = getGroupId(' test__');
        expect(gid).toBe('');

        gid = getGroupId('test__ ');
        expect(gid).toBe('');
    });

    test('positive (gid)', () => {
        let gid;

        gid = getGroupId('test__gid');
        expect(gid).toBe('gid');

        gid = getGroupId('test__gid ');
        expect(gid).toBe('gid');

        gid = getGroupId('test__1_2');
        expect(gid).toBe('1_2');

        gid = getGroupId('test__1__2');
        expect(gid).toBe('2');
    });

});

describe('mapQuestionGroups', () => {

    test('no groups', () => {
        let qq;

        qq = mapQuestionGroups([
            makeQuestion('test1'),
            makeQuestion('test2'),
        ]);
        // console.log(JSON.stringify(qq, null, 2));

        expect(qq.length).toBe(2);
        expect(qq[0].id).toBe('test1');
        expect(qq[1].id).toBe('test2');
    });

    test('real group', () => {
        let qq;

        // real group

        qq = mapQuestionGroups([
            makeQuestion('test_0'),
            makeQuestion('test0__1'),
            makeQuestion('test1__1'),
            makeQuestion('test1__2'),
            makeQuestion('test_3'),
        ]);

        expect(qq[0].id).toBe('test_0');

        expect(qq[1].length).toBe(2);
        expect(qq[1][0].id).toBe('test0__1');
        expect(qq[1][1].id).toBe('test1__1');

        expect(qq[2].length).toBe(1);
        expect(qq[2][0].id).toBe('test1__2');

        expect(qq[3].id).toBe('test_3');
    });

    test('two different adjascent groups', () => {
        let qq;

        qq = mapQuestionGroups([
            makeQuestion('test_0'),
            makeQuestion('test__1'),
            makeQuestion('test__2'),
            makeQuestion('test_1'),
        ]);

        expect(qq[0].id).toBe('test_0');

        expect(qq[1].length).toBe(1);
        expect(qq[1][0].id).toBe('test__1');

        expect(qq[2].length).toBe(1);
        expect(qq[2][0].id).toBe('test__2');

        expect(qq[3].id).toBe('test_1');
    });

});

describe('findQuestionGroupCommons', () => {

    test('no groups', () => {
        let qc;
        let group;

        qc = findQuestionGroupCommons([]);
        expect(qc).toBe('NONE');

        qc = findQuestionGroupCommons([
            makeQuestion('test1', 'TEXT'),
            makeQuestion('test2', 'NUMBER'),
        ]);
        expect(qc).toBe('NONE');

        qc = findQuestionGroupCommons([
            makeQuestion('test1', 'TEXT'),
            makeQuestion('test2', 'TEXT'),
        ]);
        expect(qc).toBe('TYPE');

        //// choices

        // different type, different choices
        group = [
            makeQuestion('test__1', 'TEXT'),
            makeQuestion('test__2', 'NUMBER'),
        ];
        group[0].choices = ['yes', 'no'];
        group[1].choices = ['yes', 'no'];
        qc = findQuestionGroupCommons(group);
        expect(qc).toBe('NONE');

        // same type, different choices
        group = [
            makeQuestion('test__1', 'TEXT'),
            makeQuestion('test__2', 'TEXT'),
        ];
        group[0].choices = ['yes', 'no'];
        qc = findQuestionGroupCommons(group);
        expect(qc).toBe('TYPE');

        // same type, differnet choices
        group = [
            makeQuestion('test__1', 'TEXT'),
            makeQuestion('test__2', 'TEXT'),
        ];
        group[0].choices = ['yes', 'no'];
        group[1].choices = ['no', 'yes'];
        qc = findQuestionGroupCommons(group);
        expect(qc).toBe('TYPE');

        // same type and choices
        group = [
            makeQuestion('test__1', 'TEXT'),
            makeQuestion('test__2', 'TEXT'),
        ];
        group[0].choices = ['yes', 'no'];
        group[1].choices = ['yes', 'no'];
        qc = findQuestionGroupCommons(group);
        expect(qc).toBe('CHOICES');
    });


});
