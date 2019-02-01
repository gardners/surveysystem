// http://eslint.org/docs/user-guide/configuring

module.exports = {

    env: {
        browser: true,
    },

    extends: 'airbnb-base',

    ////
    // add your custom rules here
    ////

    rules: {
        // allow debugger during development
        'no-debugger': process.env.NODE_ENV === 'production' ? 2 : 0,

        // indentation
        indent: ['error', 4, { 'SwitchCase': 1 }],

        // console
        'no-console': ['warn', { allow: [ 'warn' ] }],

        // off
        'no-underscore-dangle': 'off',
        'func-names': 'off',
        'spaced-comment': 'off',
        'padded-blocks': 'off',
        'arrow-body-style': 'off',
        'prefer-template': 'off',
        'no-continue': 'off',
        'object-shorthand': 'off',
        'no-param-reassign': 'off',
        'max-len': 'off',

        // warn
        'space-before-function-paren': ['warn', 'never'],
        'no-multi-spaces': 'warn',
        'comma-spacing': 'warn',
        'space-in-parens': 'warn',
        'array-bracket-spacing': 'warn',
        'space-before-blocks': 'warn',

        // error
        'comma-dangle': ['error', 'only-multiline'],
        'no-restricted-syntax': [
            'error',
            'LabeledStatement',
            'WithStatement',
        ],
        'object-curly-newline': [ "error", {
            // configuration for object patterns of destructuring assignments
            "ObjectPattern": {
                "minProperties": 10
            },
        }],
    }
};
