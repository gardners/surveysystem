import React, { useState, useEffect } from 'react';
import { useHistory } from 'react-router-dom';

import Auth from '../JwtAuth';
import { AuthContext } from '../Context';


const AuthProvider = function({ children }) {
    const [loading, setLoading] = useState('');
    const [error, setError] = useState(null);
    const [user, setUser] = useState(null);

    const is_protected = Auth.is_protected();
    const history = useHistory();

    const handleError = function(error) {
        setLoading('');
        setError(error);
        return error;
    };

    const handleUser = function(user) {
        setUser(user);
        setLoading('');
        setError(null);

        if (!user) {
            history.push('/login');
        }
        if (user) {
            history.push('/surveys');
        }
        return user;
    };

    const init = function() {
        setLoading('initalizing...');
        setError(null);

        return Auth.init()
        .then(user => handleUser(user, true, true))
        .catch(error => handleError(error));
    };

    const login = function(username, password) {
        setLoading('Logging in...');
        setError(null);

        return Auth.login(username, password)
        .then(() => Auth.init())
        .then(user => handleUser(user, true, true))
        .catch(error => handleError(error));
    };

    const logout = function() {
        setLoading('Logging out...');
        setError(null);

        return Auth.logout()
        .then(() => handleUser(null))
        .catch(error => handleError(error));
    };

    useEffect(() => {
        init();
    }, []);

    return (
        <AuthContext.Provider value={{ is_protected, user, loading, error, init, login, logout }}>
            { children }
        </AuthContext.Provider>
    );
};


export {
    AuthContext,
    AuthProvider
};
