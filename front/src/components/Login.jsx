import React, { useRef, useContext } from 'react';
import { Link, Redirect } from 'react-router-dom';

import { AuthContext } from '../Context';


const UserProfile = function() {
    const context = useContext(AuthContext);

    const user = context.user || {};
    const username = user.username || '<UNKNOWN>';

    return(
        <React.Fragment>
            <h2 className="card-title">Hello <small>{ username }</small></h2>
            <Link to="/surveys" className="btn btn-primary btn-xs">My Surveys</Link>
            <button onClick={
                (e) => {
                    e.preventDefault();
                    context.logout();
                }
            } className="btn btn-secondary btn-xs ml-4">Logout</button>
        </React.Fragment>
    );
};

UserProfile.propTypes = {};

const UserLogin = function() {
    const name = process.env.REACT_APP_SITE_NAME || 'Surveysystem';
    const usernameRef = useRef();
    const passwordRef = useRef();

    const context = useContext(AuthContext);

    const handleSubmit = (e) => {
        e.preventDefault();

        const username = usernameRef.current.value;
        const password = passwordRef.current.value;

        context.login(username, password);
    };

    return(
        <React.Fragment>
            <h2 className="card-title">Log into <small>{ name.trim() }</small></h2>

            <form onSubmit={ handleSubmit }>
                <div className="form-group">
                    <label>Username: </label>
                    <input  className="form-control" ref={ usernameRef } type="text" />
                </div>
                <div className="form-group">
                    <label>Password: </label>
                    <input className="form-control" ref={ passwordRef } type="password" />
                </div>
                <div className="form-group">
                    <button className="btn btn-primary" type="submit">Submit</button>
                </div>
            </form>
        </React.Fragment>
    );
};

UserLogin.propTypes = {};

const Login = function() {
    const context = useContext(AuthContext);

    if (!context.is_protected) {
         return (
            <Redirect to='/' />
         );
    }

    return(
        <React.Fragment>
            <h1>Account</h1>
            <div className="card">
                <div className="card-body">
                    { (context.user) ? <UserProfile /> : <UserLogin /> }
                </div>
            </div>
        </React.Fragment>
    );
};

Login.propTypes = {};

export default Login;
