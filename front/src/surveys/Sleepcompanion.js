export default {
    validators: {
        Nawakenings: function(answers) {
            return new Error('This is a test error:' + JSON. stringify(answers));
        }
    }
};
