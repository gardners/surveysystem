/**
 * Simple console.log wrapper
 * output disabled in production mode
 */
const cons = (typeof window !== 'undefined' && console in window) ? window.console : console;

const Log = {};

let key;
for (key in cons) {
    if (typeof cons[key] !== 'function') {
        continue;
    }
    if (process.env.NODE_ENV !== 'production') {
        Log[key] = function() {};
        continue;
    }
    Log[key] = cons[key].bind(cons);
}
export default Log;
