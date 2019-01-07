// getAnswer(): eturns Error or serialized value
// submit:
// - were all (required) fields filled in? (answers.length === questions.length
// - does answers collection contain Errors?
// element
//  - <element>.checkValidity() : false > Error
//  - payload serializer: Error
//  - validator: false > Error

const validateAnswer = function(element, question, ...values) {
    // TODO
    if(element && typeof element.validity !== 'undefined') {
        if (!element.validity.valid) {
            return new Error (element.validationMessage);
        }
    }

    return null;
};

export { validateAnswer };
