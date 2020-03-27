const path = require('path');
const fs = require('fs');

const { promisify } = require('util');

const _logfile_ = path.resolve('./log/player.analysis.json');
let LOGFILE = _logfile_;

const writeFileAsync = promisify(fs.writeFile);
const copyFileAsync = promisify(fs.copyFile);

const init = function(filename = '') {
    if (filename) {
        LOGFILE = path.resolve(path.join('log', filename));
    }
    return Promise.resolve(LOGFILE);
};

const save = function(data) {
    const json = JSON.stringify(data, null, 4);
    return writeFileAsync(LOGFILE, json)
        .then(() => LOGFILE);
};

const finish = function() {
    const latest = path.resolve('./log/latest.analysis.json');
    // copy log file to "latest" file, so it's easy to find
    return copyFileAsync(LOGFILE, latest)
        .then(() => latest);
};

module.exports = {
    init,
    save,
    finish,
};
