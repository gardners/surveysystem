const path = require('path');
const fs = require('fs');

const { promisify } = require('util');

const _logfile_ = path.resolve('./log/player.log.csv');
let LOGFILE = _logfile_;

const appendFileAsync = promisify(fs.appendFile);
const copyFileAsync = promisify(fs.copyFile);

const SEPARATOR = ',';

const sanitize = function(val) {
    const type = typeof val;
    if (val === null || type === 'number' || type === 'boolean') {
        return '"' + val + '"';
    }
    if (type !== 'string') {
        return JSON.stringify(val);
    }
    return '"' + val.replace('"', '\'') + '"';
};

const append = function(...args) {
    const line = args.map(val => sanitize(val)).join(SEPARATOR) + '\n';
    return appendFileAsync(LOGFILE, line);
};

const init = function(filename = '') {
    if (filename) {
        LOGFILE = path.resolve(path.join('log', filename));
    }
    return appendFileAsync(LOGFILE, '')
        .then(() => LOGFILE);
};

const finish = function() {
    const latest = path.resolve('./log/latest.log.csv');
    // copy log file to "latest" file, so it's easy to find
    return copyFileAsync(LOGFILE, latest)
        .then(() => latest);
};

module.exports = {
    init,
    append,
    finish,
};
