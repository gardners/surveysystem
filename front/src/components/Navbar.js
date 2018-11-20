//Basic Navbar made with bootstrap
import React, { Component } from 'react';

class Navbar extends Component {
    render() {
        return (
            <nav className="navbar navbar-inverse">
                <div className="container-fluid">
                    <div className="navbar-header">
                        <a className="navbar-brand" >Survey Project</a>
                    </div>
                    <ul className="nav navbar-nav">
                        <li className="active"><a >Survey List</a></li>
                        <li><a >Page 1</a></li>
                        <li><a >Page 2</a></li>
                    </ul>
                    <ul className="nav navbar-nav navbar-right">
                        <li><a ><span className="glyphicon glyphicon-user"></span> Sign Up</a></li>
                        <li><a ><span className="glyphicon glyphicon-log-in"></span> Login</a></li>
                    </ul>
                </div>
            </nav>
        );
    }
}

Navbar.propTypes = {};

export default Navbar;
