import Auth from '../JwtAuth';

const access_token = 'eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJ0b2tlbl90eXBlIjoiYWNjZXNzIiwiZXhwIjoxNjU5OTE0OTk5LCJpYXQiOjE2NTk5MTQ2OTksImp0aSI6ImVkNmI3ZGM3ZGJhODQ5YjBhZDk1YmQyZWUwNTYzOWM5IiwidXNlcl9pZCI6MX0.q6yeziwl6AZQiFXMVY8M2VbyX-V8Zx5A8UuwXaftRzU';

describe('JWT Auth', () => {

    test('parse_token', () => {
        let res;

        res = Auth.parse_token();
        expect(res).toBe(null);

        res = Auth.parse_token('');
        expect(res).toBe(null);

        res = Auth.parse_token('a.a');
        expect(res).toBe(null);

        res = Auth.parse_token(access_token);
        expect(res.token_type).toBe('access');
        expect(res.user_id).toBe(1);
    });

});
