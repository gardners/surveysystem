import React from 'react';

import { Link } from 'react-router-dom';

import { AuthContext } from '../Context';

const Profile = function() {
    return(
        <AuthContext.Consumer>
            {
                ({ user, logout }) => (
                    <div className="card">
                        <div className="card-body">
                            <h2 className="card-title">Hello <small>{ user.name }</small></h2>
                            <p>email: { user.email }</p>
                            <Link to="/surveys" className="btn btn-primary btn-xs">My Surveys</Link>
                            <button onClick={
                                (e) => {
                                    e.preventDefault();
                                    logout();
                                }
                            } className="btn btn-secondary btn-xs ml-4">Logout</button>
                        </div>
                    </div>
                )
            }
        </AuthContext.Consumer>
    );
};

Profile.propTypes = {};

const Login = function() {
    const name = process.env.REACT_APP_SITE_NAME || 'Surveysystem' ;

    return(
        <AuthContext.Consumer>
            {
                ({ user, login }) => (
                    <div className="card">
                        <div className="card-body">
                            <h2 className="card-title">Log into <small>{ name.trim() }</small></h2>
                            <button onClick={
                                (e) => {
                                    e.preventDefault();
                                    login();
                                }
                            } className="btn btn-primary btn-xs">Login</button>
                        </div>
                    </div>
                )
            }
        </AuthContext.Consumer>
    );
};

Login.propTypes = {};

/**
 * @see http://reactcommunity.org/react-modal/#usage
 */
class OAuth2Login extends React.Component {

    static contextType = AuthContext; // consume

    constructor(props) {
        super(props);

        this.state = {};
    }

    componentDidMount() {
        const { redirect_callback } = this.context;
        redirect_callback()
    }

    render() {
        const { user } = this.context;
        return (
            <React.Fragment>
                { (user) ? <Profile /> : <Login /> }
            </React.Fragment>
        );
    }
}

OAuth2Login.propTypes = {};

export default OAuth2Login;
