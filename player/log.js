const colors = {
    green: text => `\x1b[32m${text}\x1b[0m`,
    red: text => `\x1b[31m${text}\x1b[0m`,
    yellow: text => `\x1b[33m${text}\x1b[0m`,
    blue: text => `\x1b[34m${text}\x1b[0m`,
};

const log = function(status, message) {
    let m = message;
    if (message instanceof Error) {
        status = 'error';
        m = message.toString();
    }

    switch (status) {

        case 'error':
            m = colors.red(m);
            break;

        case 'success':
            m = colors.green(m);
            break;

        case 'note':
            m = colors.blue(m);
            break;

        default:
            //nothing
    }

    // eslint-disable-next-line no-console
    console.log(m);
};

const Log = {
    error: function(message, ret) {
        log('error', message);
        return ret;
    },

    success: function(message, ret) {
        log('success', message);
        return ret;
    },

    note: function(message, ret) {
        log('note', message);
        return ret;
    },

    log: function(message, ret) {
        log('log', message);
        return ret;
    },

    step: function(message, ret) {
        log('notice', `\n${colors.blue('◆')} ${message}`);
        return ret;
    },

    section: function(message, ret) {
        const hr = '='.repeat(message.length + 4);
        const hl = '= ' + colors.yellow(message) + ' =';

        const msg = [
            '\n',
            hr,
            hl,
            hr,
        ];
        log('log', msg.join('\n'));

        return ret;
    },
};

Log.colors = colors;

module.exports = Log;
