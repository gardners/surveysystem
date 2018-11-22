/**
 * @module Log
 * Simple console.log wrapper
 * output disabled in production mode
 */
import { isScalar } from './Utils';

const toConsole = (process.env.NODE_ENV !== 'production');

/**
 * Add an entry object ot a store
 * !! Forces servity "error" if message is an instance of Error
 * @param {string} severity message serverity level
 * @param {(string|number|Error)} message as string or Error instance
 * @returns {void}
 */
const add = function(severity, msg, store) {
    let message = null;
    if (msg instanceof Error) { // stringifying Error instances returns an empty object
        message = msg.toString();
        severity = 'error';
    } else {
        message = (msg === undefined || isScalar(msg)) ? msg : JSON.stringify(msg);//TODO try catch
    }

    store.push({
        severity,
        message,
    });
};

/**
 * @class
 * @classdesc Log store
 */
class Log {
    /**
     * Create a Log store
     * @returns {void}
     */
    constructor() {
        this.messages = [];
        this.subscribers = {};
    }

    /**
     * Register a subscriber callback
     * @param {string} id
     * @param {function} callback
     * @returns {void}
     */
    subscribe(id, callback) {
        this.subscribers[id] = callback;
    }

    /**
     * Remove a subscriber callback
     * @param {string} id
     * @param {function} callback
     * @returns {void}
     */
    unsubscribe(id) {
        delete (this.subscribers[id]);
    }

    /**
     * Trigger registered subscriber callbacks
     * @returns {void}
     */
    invokeSubscribers() {
        Object.keys(this.subscribers).forEach((id) => {
            this.subscribers[id](this.messages);
        });
    }

    /**
     * Log a message
     * @param {mixed} message
     * @return {void}
     */
    log(message) {
        add('log', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.log(message);
        }
    }

    /**
     * Log an error
     * @param {mixed} message
     * @returns {void}
     */
    error(message) {
        add('error', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.error(message);
        }
    }

    /**
     * Log a warning
     * @param {mixed} message
     * @returns {void}
     */
    warn(message) {
        add('warn', message, this.messages);
        this.invokeSubscribers();
        if (toConsole) {
            console.warn(message);
        }
    }

    /**
     * Log a debug message
     * @param {mixed} message
     * @returns {void}
     */
    debug(message) {
        add('debug', message, this.messages);
        if (toConsole) {
            console.debug(message);
        }
    }
}

export default new Log();
