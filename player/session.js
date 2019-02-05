const path = require('path');
const fs = require('fs');
const { promisify } = require('util');

const SessionDir = '../backend/sessions';

const readFileAsync = promisify(fs.readFile);

const readSessionFile = function(sessId) {
    const sessDir = sessId.substring(0, 4);
    const sessFile = path.resolve(`${SessionDir}/${sessDir}/${sessId}`);

    return readFileAsync(sessFile, {
        encoding: 'utf-8',
    });
};

const getlastSessionEntry = function(sessId) {
    return readSessionFile(sessId)
        .then(contents => contents.trim().split('\n').pop());
};

module.exports = {
    readSessionFile,
    getlastSessionEntry,
};
