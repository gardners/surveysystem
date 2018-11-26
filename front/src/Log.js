/**
 * @module Log
 * Simple console.log wrapper
 * output disabled in production mode
 */
import { isScalar } from './Utils';

const toConsole = (process.env.NODE_ENV !== 'production');


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
    * Add an entry object ot a store
    * !! Forces servity "error" if message is an instance of Error
    * @param {(string|number|Error)} message as string or Error instance
    * @param {string} severity message serverity level
    * @returns {void}
    */
    add(msg, severity) {
        let message = null;

        if (msg instanceof Error) { // stringifying Error instances returns an empty object
            message = msg.toString();
            severity = 'error';
        } else {
            message = (msg === undefined || isScalar(msg)) ? msg : JSON.stringify(msg);//TODO try catch
        }

        const index = this.messages.push({
            severity,
            message,
        });

        this.invokeSubscribers();

        if (toConsole) {
            const fn = (typeof console[severity] === 'function') ? severity: 'log';
            console[fn](message);
        }

        return this.messages[index - 1];
    };


    /**
     * Log a message
     * @param {mixed} message
     * @return {void}
     */
    log(message) {
        return this.add(message, 'info');
    }

    /**
     * Log an error
     * @param {mixed} message
     * @returns {void}
     */
    error(message) {
        return this.add(message, 'error');
    }

    /**
     * Log a warning
     * @param {mixed} message
     * @returns {void}
     */
    warn(message) {
        return this.add(message, 'warn');
    }

    /**
     * Log a debug message
     * @param {mixed} message
     * @returns {void}
     */
    debug(message) {
        this.add(message, 'debug');
    }


}

export default new Log();
