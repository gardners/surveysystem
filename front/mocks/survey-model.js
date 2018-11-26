const fs = require('fs');
const path = require('path');

const countAnswers = function(dir = './') {
    const contents = fs.readdirSync(path.resolve(__dirname + '/' + dir), {
        withFileTypes: true,
    });
    return contents.filter(entry => entry.isFile() && path.extname(entry.name) === '.json').length;
};

const getAnswer = function(step, dir = './') {
    const file = path.resolve(__dirname + '/' + dir) + '/' + step + '.json';
    const exists = fs.existsSync(file);
    if(exists) {
        return {
            statusCode: 200,
            statusText: 'OK',
            payload: fs.readFileSync(file, 'utf-8'),
        };
    }

    return {
        statusCode: 404,
        statusText: 'Not Found',
        payload: 'file ' + file + ' does not exist',
    };
};

module.exports = {
    countAnswers,
    getAnswer,
};

