const params =
    new URLSearchParams(
        window.location.search
    );

const username =
    params.get("username");

const $ = id =>
    document.getElementById(id);

if(username){

    $("username").value =
        username;
}

/* =========================================================
   PROFILE DATA
========================================================= */

function getValue(id){

    return $(id)
        ?.value
        .trim() || "";
}

function getProfile(){

    const techStack = [
        ...document.querySelectorAll(
            "#techStack input:checked"
        )
    ].map(
        checkbox =>
            checkbox.value
    );

    return {

        username,

        displayName:
            getValue("displayName"),

        avatar:
            getValue("avatar"),

        location:
            getValue("location"),

        bio:
            getValue("bio"),

        headline1:
            getValue("headline1"),

        headline2:
            getValue("headline2"),

        workingOn:
            getValue("workingOn"),

        learning:
            getValue("learning"),

        collaborate:
            getValue("collaborate"),

        help:
            getValue("help"),

        askMe:
            getValue("askMe"),

        pronouns:
            getValue("pronouns"),

        funFact:
            getValue("funFact"),

        portfolio:
            getValue("portfolio"),

        linkedin:
            getValue("linkedin"),

        instagram:
            getValue("instagram"),

        email:
            getValue("email"),

        techStack
    };
}

/* =========================================================
   SUBMIT
========================================================= */

$("profileForm")
    .addEventListener(
        "submit",
        async event => {

            event.preventDefault();

            const button =
                $("continueBtn");

            const status =
                $("status");

            const profile =
                getProfile();

            if(!profile.displayName){

                status.textContent =
                    "Please enter your display name.";

                status.className =
                    "error";

                return;
            }

            button.disabled =
                true;

            button.textContent =
                "Creating your profile...";

            status.textContent =
                "Creating your GitHub repository and README...";

            status.className = "";

            try {

                const response =
                    await fetch(
                        "/api/profile/setup",
                        {
                            method:"POST",

                            headers:{
                                "Content-Type":
                                    "application/json"
                            },

                            credentials:
                                "include",

                            body:
                                JSON.stringify({
                                    profile
                                })
                        }
                    );

                const data =
                    await response.json();

                if(!response.ok){

                    throw new Error(
                        data.message ||
                        "Profile setup failed."
                    );
                }

                status.textContent =
                    "Your GitHub profile repository is ready.";

                status.className =
                    "success";

                /*
                   Give the user the Profile Studio step.

                   The external Profile Studio is opened first.
                   After the user finishes there, they can return
                   to the Overview page.
                */

                const studioUrl =
                    data.profileStudioUrl;

                const overviewUrl =
                    data.overviewUrl;

                const openStudio =
                    confirm(
                        "Your GitHub profile repository and README.md are ready.\n\n" +
                        "Open Profile Studio now to customize your README?"
                    );

                if(openStudio){

                    window.location.href =
                        studioUrl;

                    return;
                }

                window.location.href =
                    overviewUrl;

            } catch(error){

                console.error(error);

                status.textContent =
                    error.message ||
                    "Something went wrong.";

                status.className =
                    "error";

                button.disabled =
                    false;

                button.textContent =
                    "Create my GitHub profile →";
            }
        }
    );