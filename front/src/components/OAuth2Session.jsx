import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { AuthContext } from '../Context';
import Provider from '../OAuth2';

import { useHistory, useLocation } from 'react-router-dom';
const { PUBLIC_URL } = process.env;

const OAuth2Login = function() {
    const name = process.env.REACT_APP_SITE_NAME || 'Surveysystem' ;

    return(
        <div className="card">
            <div className="card-body">
                <h2 className="card-title">Log into <small>{ name.trim() }</small></h2>
                <p className="card-text">Some quick example text to build on the card title and make up the bulk of the card's content.</p>
                <a href={ Provider.loginUrl() } className="btn btn-primary btn-xs">Login</a>
            </div>
        </div>
    );
};

OAuth2Login.propTypes = {};

const OAuth2UserProfile = function({ user, logout }) {

    const history = useHistory();
    const location = useLocation();

    // redirect to previous page or root @component ProtectedRouter
    const state = location.state || {}
    const from = state.from || {
        from: {
            pathname: PUBLIC_URL
        }
    };

    return(
        <div className="card">
            <div className="card-body">
                <h2 className="card-title">Hello <small>{ user }</small></h2>
                    <button onClick={ (e) => {
                            e.preventDefault();
                            history.replace(from);
                    } } className="btn btn-success btn-xs">Back to previous page</button>
                    <button className="btn btn-sm btn-secondary" onClick={ logout }>Logout</button>
            </div>
        </div>
    );
};

OAuth2UserProfile.defaultProps = {
    user: '',
    logout: () => {},
};

OAuth2UserProfile.propTypes = {
    user: PropTypes.string.isRequired,
    logout: PropTypes.func.isRequired,
};


class OAuth2Session extends Component {

    static contextType = AuthContext; // consume

    constructor(props) {
        super(props);
        this.state = {
            user: '',
            loading: '',
            error: null,
        };
    }

    componentDidMount() {
        this.login();
    }

    error(error) {
        console.log(error);
        this.setState({
            loading: '',
            error: error,
        });
    }

    getUser() {
        const access_token = Provider.accessToken();
        return Provider.getUser(access_token)
        .then((res) => {

            // grab an available user prop
            let user = '<logged_in>';
            if (typeof res.name !== 'undefined' ) {
                user = res.name;
            }
            if (!user && typeof res.email !== 'undefined' ) {
                user = res.email;
            }
            if (!user && typeof res.id !== 'undefined' ) {
                user = res.id;
            }
            if (!user && typeof res.uid !== 'undefined' ) {
                user = res.uid;
            }

            this.context.updateUser(user);
            this.setState({
                user,
                loading: '',
                error: null,
            });

            return res;
        })
        .catch(() => {
            this.setState({
                user: '',
                loading: '',
                error: '',
            });
            this.context.updateUser('');
            return Promise.reject('Login required');
        })
    }

    login() {
        const urlParams = new URLSearchParams(window.location.search);
        const code = urlParams.get('code');

        // case: redirection from /authorize
        if (code) {
            return Provider.login(code)
            .then(() => this.getUser())
            .catch(error => this.error(error));
        }

       // case:  access_token
        const access_token = Provider.accessToken();
        if (access_token) {
            return Provider.introspect(access_token)
            .then((res) => {
                console.log(res);
                // case: valid access_token
                if (res.active) {
                    return this.getUser();
                }

                // case: refresh_ token
                const refresh_token = Provider.refreshToken();
                if(refresh_token) {
                    return Provider.refresh(refresh_token)
                    .then(() => this.getUser());
                }
            })
        }

        return Promise.reject('Login required');
    }

    logout () {
        return Provider.logout()
        .then(() => this.setState({
            user: '',
            loading: '',
            error: null,
        }))
        .catch(error => this.error(error));
    }

    render(){
        const { user, error } = this.state;

        return(
            <React.Fragment>
                { error && <div className="alert alert-danger">{ (error instanceof Error) ? error.toString() : JSON.stringify(error, null, 4) }</div> }

                {
                    (user) ?
                        <OAuth2UserProfile user={ user } logout= { this.logout.bind(this) } />
                    :
                        <OAuth2Login />
                }
            </React.Fragment>
        );
    }
};

OAuth2Session.propTypes = {};

export default OAuth2Session;
