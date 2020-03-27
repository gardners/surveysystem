const path = require('path');
const fs = require('fs');
const { promisify } = require('util');
const imports = require('esm')(module);

const _libanswer = imports('./ext/Answer');
const Answer = (typeof _libanswer.default !== 'undefined') ? _libanswer.default : _libanswer;

const SessionDir = '../backend/sessions';

const readFileAsync = promisify(fs.readFile);

const readSessionFile = function(sessId) {
    const sessDir = sessId.substring(0, 4);
    const sessFile = path.resolve(`${SessionDir}/${sessDir}/${sessId}`);

    return readFileAsync(sessFile, {
        encoding: 'utf-8',
    });
};

const parseSessionFile = function(sessionFile) {
    return readFileAsync(sessionFile, { encoding: 'utf-8' })
        .then(buf => buf.toString().trim().split('\n'))// remove empty lines at the end
        .then((lines) => {
            let isHeader = true;
            const answers = {};
            lines.forEach((line) => {
                const answer = Answer.deserialize(line);

                if (answer instanceof Error) {
                    if (isHeader) {
                        return;
                    }
                    throw answer;
                }
                isHeader = false;
                answers[answer.uid] = answer;
            });
            return answers;
        });
};

const getlastSessionEntry = function(sessId) {
    return readSessionFile(sessId)
        .then(contents => contents.trim().split('\n').pop());
};

module.exports = {
    parseSessionFile,
    readSessionFile,
    getlastSessionEntry,
};
